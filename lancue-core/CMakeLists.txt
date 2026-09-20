# lancue-core: the background daemon (§2.1). Phase 1 switches it to a
# headless subsystem on Windows (no visible console window on launch, per
# "clean background execution") — deferred from Phase 0 specifically to
# this phase; see the roadmap Learnings log. macOS/Linux executables have
# no equivalent "console subsystem" flag to flip: running headless there is
# a matter of how the process is launched (a LaunchAgent/systemd unit in
# Phase 12), not a build setting, so no branch is needed for them.
#
# Phase 7 adds Widgets/Gui and switches main.cpp from QCoreApplication to
# QApplication (see main.cpp's own comment) — the daemon now owns real
# QWidget windows (the toast, and Phase 8's follower window) per §2.1's
# own architecture, not just a headless event loop. It stays
# WIN32_EXECUTABLE (no console) below regardless — a GUI Qt application and
# a console-less Windows subsystem are orthogonal; nothing about adding
# Widgets requires a console window back.

find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Network Gui Widgets)

add_executable(lancue-core
    src/main.cpp
    src/feature_registry/feature_registry.cpp
    src/layout_watcher_feature/layout_watcher_feature.cpp
    src/global_hotkey_feature/global_hotkey_feature.cpp
    src/conversion_feature/conversion_feature.cpp
    src/toast_feature/toast_feature.cpp
    src/toast_feature/toast_window.cpp
    src/mouse_watcher_feature/mouse_watcher_feature.cpp
    src/follower_feature/follower_feature.cpp
    src/follower_feature/follower_window.cpp
    src/ui/fonts.cpp
    ${CMAKE_SOURCE_DIR}/resources/fonts/fonts.qrc
)

if(WIN32)
    set_target_properties(lancue-core PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(lancue-core
    PRIVATE
        # Mirrors core-lib's own src/ include dir (see core-lib/CMakeLists.txt):
        # lets src/feature_registry/feature_registry.h and
        # src/layout_watcher_feature/layout_watcher_feature.h #include
        # "feature.h" by its short path regardless of which subdirectory
        # they live in, instead of every feature folder needing "../feature.h".
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lancue-core
    PRIVATE
        core-lib
        Qt6::Core
        Qt6::Network
        Qt6::Gui
        Qt6::Widgets
)

# Now a Widgets app like lancue-settings (see that target's own comment on
# why this call exists) — Phase 7 is what made that true; the toast is a
# real QWidget, not a console-only feature anymore.
lancue_deploy_qt_platform_plugin(lancue-core)
