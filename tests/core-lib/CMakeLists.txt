# Unit tests for core-lib modules. Framework decision (Phase 1, recorded in
# the roadmap's Learnings log §6.2): Catch2 via vcpkg — header-light,
# integrates with ctest through catch_discover_tests, and its BDD-ish
# TEST_CASE/SECTION style reads well for the small, focused cases these
# modules need (framing edge cases, dispatch routing, event delivery).
#
# Phase 2 switches from Catch2::Catch2WithMain to a hand-rolled main
# (main_test.cpp) that constructs a QCoreApplication before handing off to
# Catch2 — LayoutChangeDebouncer's tests need a real Qt event loop for its
# QTimer to fire, which Catch2WithMain's generated main() doesn't provide.

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(core-lib-tests
    main_test.cpp
    ipc_message_test.cpp
    ipc_dispatcher_test.cpp
    event_bus_test.cpp
    layout_change_debouncer_test.cpp
    hotkey_combo_test.cpp
    settings_test.cpp
    selection_clipboard_bridge_test.cpp
    conversion_engine_test.cpp
    language_test.cpp
    toast_position_test.cpp
    toast_monitor_mode_test.cpp
    layout_color_palette_test.cpp
    app_theme_mode_test.cpp
)

target_link_libraries(core-lib-tests
    PRIVATE
        core-lib
        Catch2::Catch2
)

include(CTest)
include(Catch)
catch_discover_tests(core-lib-tests)
