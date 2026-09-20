# lancue-settings: tray icon + settings window. UI only, no business logic (§2.1).
# Stays a console subsystem app for now — unlike lancue-core, nothing in
# Phase 1 requires hiding its window/console, since it has no persistent
# background-run requirement yet (that arrives with the tray icon in
# Phase 9, which is when the WIN32/MACOSX_BUNDLE switch belongs here too).

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Network)

add_executable(lancue-settings
    src/main.cpp
)

target_link_libraries(lancue-settings
    PRIVATE
        core-lib
        Qt6::Widgets
        Qt6::Network
)

# Widgets apps need the "platforms" plugin at runtime (see cmake/qt-deploy-platform-plugin.cmake
# for why vcpkg doesn't put it there automatically). lancue-core doesn't call this — it's
# QCoreApplication-only and never loads a platform plugin.
lancue_deploy_qt_platform_plugin(lancue-settings)
