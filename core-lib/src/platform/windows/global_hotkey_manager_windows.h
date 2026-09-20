#pragma once

#include <Windows.h>

#include <QAbstractNativeEventFilter>
#include <QHash>

#include "corelib/platform/IGlobalHotkeyManager.h"

namespace lancue::platform::windows {

// RegisterHotKey-based implementation. Unlike Phase 2's keyboard-layout
// watcher (which had to abandon its "documented modern mechanism" for a
// low-level hook — see the Phase 2 Learnings entries), RegisterHotKey is
// the right tool from the start here: it claims a specific, currently-
// unclaimed combination for this process, which is exactly what an
// application-defined hotkey is (the Phase 2 Learnings entry on
// RegisterHotKey vs. WH_KEYBOARD_LL already reasons through this
// distinction).
//
// RegisterHotKey(NULL, ...) posts WM_HOTKEY as a *thread* message, not a
// window message — it needs no HWND, but it does need something pumping
// this thread's message queue and looking at raw MSG values, which a
// plain Qt event loop doesn't expose by default. QAbstractNativeEventFilter
// is Qt's documented mechanism for exactly this: it's installed on
// QCoreApplication and sees every native MSG before Qt's own event
// dispatcher does, without requiring a window of our own the way a
// classic WndProc-based approach would.
//
// RegisterHotKey identifies each registration by a small integer, not a
// string, so this class maintains its own id<->integer mapping in both
// directions (§ "supports any number of simultaneously-registered
// hotkeys" in the interface's own comment) — the integer is purely an
// internal implementation detail of talking to RegisterHotKey/WM_HOTKEY;
// callers only ever see the string HotkeyId.
class GlobalHotkeyManagerWindows final : public IGlobalHotkeyManager, public QAbstractNativeEventFilter {
public:
    GlobalHotkeyManagerWindows();
    ~GlobalHotkeyManagerWindows() override;

    void setCallback(HotkeyPressedCallback callback) override;
    bool registerHotkey(const HotkeyId& id, const HotkeyCombo& combo) override;
    void unregisterHotkey(const HotkeyId& id) override;
    void stop() override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    HotkeyPressedCallback m_callback;
    bool m_filterInstalled = false;

    // RegisterHotKey's own id space is a plain int; this class hands out
    // sequential ids starting at 1 (0 is avoided only to keep the space
    // trivially distinguishable from "not found" in ad-hoc debugging, not
    // because 0 has any special meaning to the Win32 API itself).
    int m_nextWinId = 1;
    QHash<HotkeyId, int> m_idToWinId;
    QHash<int, HotkeyId> m_winIdToId;
};

} // namespace lancue::platform::windows
