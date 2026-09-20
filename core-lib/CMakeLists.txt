# core-lib holds everything lancue-core and lancue-settings both need:
# settings schema, IPC protocol, platform-abstraction interfaces, layout
# maps, logging. See §2.1/§3 of the roadmap.
#
# Phase 1 fills in: the IPC message envelope + transport (ipc/), the
# in-process EventBus (eventbus/), single-instance enforcement
# (lifecycle/), and a minimal file logger (logging/) needed once
# lancue-core drops its console window. The settings schema (settings/)
# stays empty until the phase that needs it (§4.3 — no interface without
# two call sites yet).
#
# Phase 2 fills in platform/: IKeyboardLayoutWatcher (§3) plus exactly one
# OS-specific .cpp compiled in below, selected by CMake rather than an
# #ifdef at any call site (§4.4). None of the new Phase 2 classes are
# QObjects (see layout_change_debouncer.h's own comment on why), so none
# of them need adding to the explicit AUTOMOC list below.
#
# Phase 3 adds IGlobalHotkeyManager the same way: one OS-specific .cpp
# selected below, plus hotkey_combo.cpp (HotkeyCombo::toString(), OS-
# agnostic, alongside layout_change_debouncer.cpp for the same reason).
# GlobalHotkeyManagerWindows *is* a QObject-adjacent type in the sense
# that it derives from QAbstractNativeEventFilter, but that base isn't
# Q_OBJECT/moc-generated (it's a plain interface class installed via
# QCoreApplication::installNativeEventFilter), so it doesn't need adding
# to the explicit AUTOMOC list below either.
#
# Phase 4 adds settings/: settings_schema.cpp (versioned struct + JSON
# (de)serialization, OS-agnostic) and settings_manager.cpp (load/save +
# live-preview mutators). Neither is a QObject (see settings_manager.h's
# own comment on why), so neither needs the explicit AUTOMOC list below.
# Six new handler .cpp files land under ipc/handlers/ alongside the
# existing ones, registered the same way through registration.cpp.
#
# Phase 5 adds IClipboardController/IInputSimulator: one OS-specific .cpp
# pair selected below (§4.4), plus selection_clipboard_bridge.cpp (the
# OS-agnostic orchestrator composing them, alongside hotkey_combo.cpp and
# layout_change_debouncer.cpp for the same reason). Two more dev-only
# handler .cpp files (debug_capture_selection_handler.cpp,
# debug_paste_replacement_handler.cpp) land under ipc/handlers/ the same
# way Phase 4's did. None of these are QObjects, so none need the
# explicit AUTOMOC list below either.
#
# Phase 6 adds conversion/: layout_char_maps.cpp (the data tables, §4.4 —
# "a new layout pair is a new table, not new code") and
# conversion_engine.cpp (the OS-agnostic char-by-char conversion logic
# that reads those tables, alongside hotkey_combo.cpp/
# selection_clipboard_bridge.cpp for the same reason: pure logic composed
# from data, not a platform/<os> implementation). Neither is a QObject,
# so neither needs the explicit AUTOMOC list below. Reachable from both
# lancue-core's ConversionFeature and lancue-cli directly, since both
# link core-lib — see lancue-cli/CMakeLists.txt for why lancue-cli links
# core-lib directly rather than going over IPC.
#
# Phase 7 adds i18n/language.cpp: the AppLanguage enum (settings_schema.cpp
# now persists it), RTL/LTR direction resolution, and localized
# layout-display-name/toast-string lookups — plain QString/enum logic, no
# QtGui dependency (see hotkey_combo.cpp's own comment on why core-lib
# stays off QtGui), so it costs nothing extra to link into lancue-cli or
# any future non-GUI consumer. The bundled Manrope/Vazirmatn *font files*
# and the code that loads them (QFontDatabase, which does need QtGui) stay
# local to lancue-core instead (see lancue-core/src/ui/fonts.cpp's own
# comment) — only one executable actually renders text with them yet.
#
# layout_color_palette.cpp is here for the same reason — plain hex
# strings, no QtGui.
#
# Phase 8 adds platform/IMouseMoveHook.h: one OS-specific .cpp selected
# below (§4.4), same pattern as Phase 2/3/5's platform interfaces. Unlike
# those, all three platform implementations land as real code in this
# same phase rather than Windows-plus-stubs (§4.12, which postdates
# those earlier phases) — see IMouseMoveHook.h's own comment. The
# Linux implementation needs libX11 + libXi as system packages (see the
# find_package(X11) call below); the macOS one needs the
# ApplicationServices framework (see the target_link_libraries call
# below). Neither is a QObject, so neither needs the explicit AUTOMOC
# list above.

add_library(core-lib STATIC
    src/ipc/message.cpp
    src/ipc/frame_reader.cpp
    src/ipc/dispatcher.cpp
    src/ipc/socket_name.cpp
    src/ipc/ipc_server.cpp
    src/ipc/ipc_client.cpp
    src/ipc/handlers/registration.cpp
    src/ipc/handlers/ping_handler.cpp
    src/ipc/handlers/set_toast_duration_handler.cpp
    src/ipc/handlers/set_toast_opacity_percent_handler.cpp
    src/ipc/handlers/set_follower_duration_handler.cpp
    src/ipc/handlers/set_follower_enabled_handler.cpp
    src/ipc/handlers/set_follower_opacity_percent_handler.cpp
    src/ipc/handlers/set_hotkey_handler.cpp
    src/ipc/handlers/remove_hotkey_handler.cpp
    src/ipc/handlers/set_layout_conversion_pairs_handler.cpp
    src/ipc/handlers/set_layout_color_assignment_handler.cpp
    src/ipc/handlers/set_toast_enabled_handler.cpp
    src/ipc/handlers/set_app_language_handler.cpp
    src/ipc/handlers/set_toast_position_handler.cpp
    src/ipc/handlers/set_toast_monitor_mode_handler.cpp
    src/ipc/handlers/set_app_theme_mode_handler.cpp
    src/ipc/handlers/set_toast_monitor_index_handler.cpp
    src/ipc/handlers/get_settings_handler.cpp
    src/ipc/handlers/save_settings_handler.cpp
    src/ipc/handlers/debug_capture_selection_handler.cpp
    src/ipc/handlers/debug_paste_replacement_handler.cpp
    src/eventbus/event_bus.cpp
    src/lifecycle/single_instance_guard.cpp
    src/logging/logger.cpp
    src/platform/layout_change_debouncer.cpp
    src/platform/hotkey_combo.cpp
    src/platform/selection_clipboard_bridge.cpp
    src/settings/settings_schema.cpp
    src/settings/settings_manager.cpp
    src/conversion/layout_char_maps.cpp
    src/conversion/conversion_engine.cpp
    src/i18n/language.cpp
    src/ui/toast_position.cpp
    src/ui/toast_monitor_mode.cpp
    src/ui/layout_color_palette.cpp
    src/ui/app_theme_mode.cpp
    # Q_OBJECT-bearing headers, listed explicitly even though none of them
    # are compiled directly (CMake treats a .h passed here as a non-built
    # HEADER_FILE_ONLY source). AUTOMOC's include-graph scan *should* find
    # these purely from event_bus.cpp/ipc_server.cpp/ipc_client.cpp
    # #include-ing them, but they live under include/ rather than next to
    # their .cpp — listing them directly as target sources is the
    # documented, reliable way to guarantee moc runs on them regardless of
    # that scan, and is cheap insurance against exactly the LNK2001/LNK2019
    # "missing metaObject/qt_metacast/qt_metacall" errors this phase hit
    # once already (stale AUTOMOC dependency cache after these files were
    # added across separate zip uploads — see the Phase 1 Learnings log).
    include/corelib/eventbus/event_bus.h
    include/corelib/ipc/ipc_server.h
    include/corelib/ipc/ipc_client.h
)

# Exactly one platform's IKeyboardLayoutWatcher/IGlobalHotkeyManager
# implementation is compiled in per OS, chosen here once rather than via
# #ifdef at any call site (§4.4) — createKeyboardLayoutWatcher() and
# createGlobalHotkeyManager() are what every caller actually uses.
if(WIN32)
    target_sources(core-lib PRIVATE
        src/platform/windows/keyboard_layout_watcher_windows.cpp
        src/platform/windows/global_hotkey_manager_windows.cpp
        src/platform/windows/clipboard_controller_windows.cpp
        src/platform/windows/input_simulator_windows.cpp
        src/platform/windows/mouse_move_hook_windows.cpp
    )
elseif(APPLE)
    target_sources(core-lib PRIVATE
        src/platform/macos/keyboard_layout_watcher_macos.cpp
        src/platform/macos/global_hotkey_manager_macos.cpp
        src/platform/macos/clipboard_controller_macos.cpp
        src/platform/macos/input_simulator_macos.cpp
        src/platform/macos/mouse_move_hook_macos.cpp
    )
    # ApplicationServices provides the CGEventTap family used by
    # mouse_move_hook_macos.cpp (see that file's own comment).
    target_link_libraries(core-lib PRIVATE "-framework ApplicationServices")
else()
    target_sources(core-lib PRIVATE
        src/platform/linux/keyboard_layout_watcher_linux.cpp
        src/platform/linux/global_hotkey_manager_linux.cpp
        src/platform/linux/clipboard_controller_linux.cpp
        src/platform/linux/input_simulator_linux.cpp
        src/platform/linux/mouse_move_hook_linux.cpp
    )
    # X11 (and its XInput2/"Xi" extension) is deliberately consumed as a
    # system library via CMake's own FindX11 module rather than through
    # vcpkg: vcpkg's own Linux documentation treats X11/Wayland as
    # provided by the system, not the package manager, since they're
    # tied to whatever display server is actually installed on the
    # machine — the same reason Qt6 itself links system X11 on Linux
    # rather than a vcpkg-provided copy. Needs the libx11-dev and
    # libxi-dev system packages installed (apt: libx11-dev libxi-dev) —
    # not yet wired into any CI workflow in this archive; flag that to
    # whoever next touches Linux CI.
    find_package(X11 REQUIRED COMPONENTS Xi)
    target_link_libraries(core-lib PRIVATE X11::X11 X11::Xi)
endif()

target_include_directories(core-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        # Lets files under src/ include each other with short paths (e.g.
        # "ipc/handlers/ping_handler.h") without exposing those
        # handler-internal headers as part of core-lib's public API.
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Network)

target_link_libraries(core-lib
    PUBLIC
        nlohmann_json::nlohmann_json
        Qt6::Core
        Qt6::Network
)

# No Windows-specific target_link_libraries needed anymore:
# keyboard_layout_watcher_windows.cpp now uses a WH_KEYBOARD_LL hook
# (User32, already in MSVC's default library list) instead of TSF —
# the earlier msctf/ole32/INITGUID setup this replaced is documented in
# the Phase 2 Learnings entries, for whenever TSF is revisited.
