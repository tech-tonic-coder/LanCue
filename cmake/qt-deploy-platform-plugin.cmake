# Qt's "platforms" plugin (qwindows.dll / libqcocoa.dylib / libqxcb.so) is
# never picked up automatically when Qt comes from vcpkg — nothing puts it
# next to the .exe, and running without it fails immediately with:
#   "Could not find the Qt platform plugin ... in ''"
# This only affects executables that create a QApplication / QGuiApplication
# (i.e. anything using Widgets or QML) *and* link a dynamically-built Qt6.
# lancue-core has been a real QApplication (not QCoreApplication) since
# Phase 7 (see main.cpp's own comment) — updated here after that stopped
# being true, having gone unnoticed until a real CI run actually exercised
# every target rather than just the ones a local dev session happens to
# touch.
#
# Call lancue_deploy_qt_platform_plugin(<target>) right after the target's
# add_executable()/target_link_libraries() calls for any GUI executable.
#
# The exact on-disk layout of vcpkg's Qt install (whether debug plugins get
# a "d"/"_debug" suffix, whether "plugins/" sits next to "bin/" or somewhere
# else) isn't something to hardcode and hope — different vcpkg/Qt versions
# have disagreed on this before. Instead this locates the real file at
# configure time with find_file() and fails configure with a clear message
# (not a cryptic build-time copy error) if it can't be found.
#
# None of the above applies to a *statically*-linked Qt6 at all, which is
# what vcpkg's own default triplets actually produce on Linux/macOS
# (x64-linux, arm64-osx — both VCPKG_LIBRARY_LINKAGE static; only
# x64-windows defaults to dynamic) — confirmed directly by a real CI run:
# Qt6::Core resolved to a plain qwindows.dll on windows-x64-debug, but to
# libQt6Core.a on both linux-x64-debug and macos-arm64-debug, with no
# standalone plugin file to find at all there. A static Qt6 needs no
# deployed plugin file in the first place: Qt's own CMake integration
# links a sane default set of static plugins automatically for every Qt
# module a target links against (confirmed directly against Qt's own
# qt_import_plugins documentation — that command exists only to
# *customize* the default set, never to enable it in the first place), so
# linking Qt6::Widgets — which every GUI target here already does —
# already pulls the platform integration plugin (QXcbIntegrationPlugin /
# QCocoaIntegrationPlugin) and its Q_IMPORT_PLUGIN stub source in on its
# own. See the STATIC_LIBRARY branch below.
#
# Switching Linux/macOS to a dynamic vcpkg triplet instead (to make the
# existing file-deploy logic apply uniformly) was considered and rejected:
# vcpkg ships no built-in dynamic triplet for Linux/macOS at all (Microsoft's
# own docs show getting one means hand-writing a custom overlay triplet),
# and qtbase specifically has a documented history of failing to build at
# all on a custom Linux dynamic triplet (microsoft/vcpkg#9847). Supporting
# vcpkg's own well-tested default (static) properly, rather than fighting
# it, is the safer fix.

function(lancue_deploy_qt_platform_plugin target)
    get_target_property(_lancue_qt_core_type Qt6::Core TYPE)
    if(_lancue_qt_core_type STREQUAL "STATIC_LIBRARY")
        message(STATUS
            "lancue_deploy_qt_platform_plugin: ${target} links a static Qt6::Core — "
            "the platform plugin is already statically linked in via Qt6::Widgets "
            "(see this function's own comment), nothing to deploy."
        )
        return()
    endif()

    get_target_property(_lancue_qt_core_loc Qt6::Core LOCATION)
    if(NOT _lancue_qt_core_loc)
        message(FATAL_ERROR "lancue_deploy_qt_platform_plugin: could not resolve Qt6::Core's location for target ${target}.")
    endif()
    get_filename_component(_lancue_qt_bin_dir "${_lancue_qt_core_loc}" DIRECTORY)
    get_filename_component(_lancue_qt_prefix_dir "${_lancue_qt_bin_dir}" DIRECTORY)

    if(WIN32)
        # Confirmed on 2026-07-22 against a real vcpkg install: the plugin
        # filename itself has no "d" suffix in either config — debug and
        # release are both literally "qwindows.dll", distinguished only by
        # which directory (".../debug/Qt6/plugins/platforms" vs
        # ".../Qt6/plugins/platforms") they live in. "qwindowsd.dll" is kept
        # first in this list only as a defensive fallback in case a future
        # vcpkg/Qt version reintroduces that Qt-upstream convention.
        set(_lancue_plugin_names qwindowsd.dll qwindows.dll)
    elseif(APPLE)
        set(_lancue_plugin_names libqcocoa_debug.dylib libqcocoa.dylib)
    elseif(UNIX)
        set(_lancue_plugin_names libqxcb.so)
    else()
        message(WARNING "lancue_deploy_qt_platform_plugin: unrecognized platform for target ${target}, skipping.")
        return()
    endif()

    # Search a handful of layouts vcpkg/Qt have used across versions rather
    # than assuming just one. "<prefix>/Qt6/plugins/platforms" is confirmed
    # correct for this vcpkg qtbase build (2026-07-22); the others are kept
    # as fallbacks for other vcpkg/Qt versions.
    find_file(LANCUE_QT_PLATFORM_PLUGIN_${target}
        NAMES ${_lancue_plugin_names}
        PATHS
            "${_lancue_qt_prefix_dir}/Qt6/plugins/platforms"
            "${_lancue_qt_prefix_dir}/plugins/platforms"
            "${_lancue_qt_prefix_dir}/../plugins/platforms"
        NO_DEFAULT_PATH
    )

    if(NOT LANCUE_QT_PLATFORM_PLUGIN_${target})
        message(FATAL_ERROR
            "lancue_deploy_qt_platform_plugin: could not find a Qt platform plugin "
            "(looked for: ${_lancue_plugin_names}) under '${_lancue_qt_prefix_dir}'. "
            "Qt6::Core resolved to '${_lancue_qt_core_loc}'. Find the plugin manually "
            "(e.g. `Get-ChildItem -Recurse -Filter qwindows*` under your vcpkg_installed "
            "folder) and either report the real path back so this search list can be "
            "corrected, or add it as an extra PATHS entry above."
        )
    endif()

    message(STATUS "lancue_deploy_qt_platform_plugin: ${target} -> ${LANCUE_QT_PLATFORM_PLUGIN_${target}}")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${target}>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LANCUE_QT_PLATFORM_PLUGIN_${target}}"
            "$<TARGET_FILE_DIR:${target}>/platforms/"
        COMMENT "Deploying Qt platform plugin for ${target}"
        VERBATIM
    )

    # Belt-and-suspenders fix for vcpkg-built Qt: without a qt.conf, Qt falls
    # back to its compiled-in install prefix (a path from the vcpkg build
    # machine) to decide where "platforms/" should be, and on some vcpkg/Qt
    # builds that compiled-in default doesn't reliably fall through to
    # applicationDirPath() the way windeployqt-deployed apps expect. Writing
    # an explicit qt.conf removes the ambiguity: it tells Qt outright to use
    # the executable's own directory as the prefix, so "<exeDir>/platforms"
    # is unambiguously where it looks.
    file(GENERATE
        OUTPUT "$<TARGET_FILE_DIR:${target}>/qt.conf"
        CONTENT "[Paths]\nPrefix = .\n"
    )
endfunction()
