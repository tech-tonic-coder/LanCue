#pragma once

#include <Windows.h>

#include "corelib/platform/IClipboardController.h"

namespace lancue::platform::windows {

// AddClipboardFormatListener-based implementation. Same reasoning
// category as Phase 2's WH_KEYBOARD_LL-over-TSF decision (§4.9): this is
// the mechanism the official Win32 clipboard-viewer documentation itself
// recommends and real clipboard-manager utilities actually use — it
// replaced the older WM_DRAWCLIPBOARD clipboard-viewer-chain approach
// (which required each listener to forward the message to the next
// window in the chain and would silently break the whole chain if one
// listener misbehaved or closed uncleanly), so there was no "documented
// modern mechanism vs. what real software uses" gap to fall into here.
//
// AddClipboardFormatListener delivers WM_CLIPBOARDUPDATE to a specific
// HWND, unlike RegisterHotKey's thread-message-based WM_HOTKEY
// (GlobalHotkeyManagerWindows's own comment) — so this class owns a
// hidden message-only window (HWND_MESSAGE) purely to receive that one
// message, with its own minimal WNDCLASS/WndProc rather than going
// through QAbstractNativeEventFilter: a message-only window's messages
// still flow through this thread's normal GetMessage/DispatchMessage
// loop (which Qt's event dispatcher already pumps), DispatchMessage just
// routes them to whatever WndProc the window was registered with, so a
// small dedicated WndProc is simpler here than filtering every native
// message the process receives for one that happens to be
// WM_CLIPBOARDUPDATE.
class ClipboardControllerWindows final : public IClipboardController {
public:
    ClipboardControllerWindows();
    ~ClipboardControllerWindows() override;

    void setCallback(ClipboardChangedCallback callback) override;
    bool start() override;
    void stop() override;
    bool hasText() const override;
    std::optional<QString> readText() const override;
    bool writeText(const QString& text) override;

public:
    // Public rather than private: assigned directly as a WNDPROC function
    // pointer (`wc.lpfnWndProc = ClipboardControllerWindows::wndProc;`)
    // from ensureWindowClassRegistered(), a free function in this class's
    // own .cpp — not a member or friend of this class, so it needs actual
    // access, not just the ability to take this function's address from
    // inside the class itself. Still not part of IClipboardController or
    // meant to be called by anything other than the Win32 message loop
    // itself; callers should use start()/stop()/setCallback() instead.
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    ClipboardChangedCallback m_callback;
    HWND m_hwnd = nullptr;
    bool m_listenerInstalled = false;
};

} // namespace lancue::platform::windows
