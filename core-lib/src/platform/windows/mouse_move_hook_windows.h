#pragma once

#include <Windows.h>

#include <QThread>

#include "corelib/platform/IMouseMoveHook.h"

namespace lancue::platform::windows {

// WH_MOUSE_LL-based implementation — same class of mechanism as
// keyboard_layout_watcher_windows.cpp's WH_KEYBOARD_LL hook, and
// installed/pumped the same way: on its own dedicated thread (see this
// .cpp's own MouseHookThread), never the main thread, because
// SetWindowsHookEx's own documented contract requires *some* message
// loop pumping the installing thread, and WH_MOUSE_LL's callback runs
// nested inside that pump exactly like WH_KEYBOARD_LL's does.
//
// Why WH_MOUSE_LL and not RegisterRawInputDevices (WM_INPUT): raw input
// mouse data is relative motion deltas on real hardware (confirmed via
// research — the MOUSE_MOVE_ABSOLUTE flag real USB mice/touchpads
// report is effectively never set; that flag is meant for genuinely
// absolute-position devices like tablets/RDP sessions), so recovering
// an absolute screen position from it means accumulating deltas
// yourself — a well-documented source of drift against the OS's own
// actual cursor position over time. This interface deliberately never
// reads a position from the hook at all (see IMouseMoveHook.h's own
// comment), so that drift risk is moot either way — but it's further
// confirmation WH_MOUSE_LL, not Raw Input, is the API actually meant
// for "the cursor has a new position," which is what this hook exists
// to notice.
//
// Same antivirus-flagging caveat as WH_KEYBOARD_LL already documented
// for this project (heuristic engines flag the *technique*, since a
// keylogger could use the same hook type — an already-accepted,
// documented trade-off for the keyboard hook this project ships). This
// callback only ever checks whether the message is WM_MOUSEMOVE to
// decide whether to invoke setCallback()'s callback — it never reads
// MSLLHOOKSTRUCT's coordinates or any button/click state.
class MouseMoveHookWindows final : public IMouseMoveHook {
public:
    MouseMoveHookWindows();
    ~MouseMoveHookWindows() override;

    void setCallback(MouseMovedCallback callback) override;
    bool start() override;
    void stop() override;

private:
    static LRESULT CALLBACK lowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam);

    // SetWindowsHookEx's callback is a plain function pointer with no
    // room for a `this` argument — same single-active-instance approach
    // as KeyboardLayoutWatcherWindows::s_activeInstance (this project
    // only ever constructs one of each platform singleton per process).
    static MouseMoveHookWindows* s_activeInstance;

    MouseMovedCallback m_callback;

    // Owns the SetWindowsHookEx call and the message pump it needs (see
    // this .cpp's own MouseHookThread) — started in start(), stopped
    // and deleted in stop(). A plain QThread* (not the concrete,
    // .cpp-local MouseHookThread type) so this header doesn't need to
    // declare that type.
    QThread* m_hookThread = nullptr;
};

} // namespace lancue::platform::windows
