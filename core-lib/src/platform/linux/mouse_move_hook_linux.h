#pragma once

#include <thread>

#include "corelib/platform/IMouseMoveHook.h"

namespace lancue::platform::linux_ {

// XInput2 XI_RawMotion-based implementation on X11 — matches
// keyboard_layout_watcher_linux.h's own stated baseline for this
// project (XKB on X11; Wayland has no compositor-agnostic equivalent
// yet, same caveat here — see start()'s own comment for the runtime
// check this implementation makes before relying on it). XI_RawMotion
// itself carries no absolute position — confirmed via research, this is
// XInput2's own well-documented behavior, not a limitation specific to
// this implementation — so, consistent with this project's own
// IMouseMoveHook.h contract (see that header's comment on why no
// platform implementation hands back a position of its own), this class
// only ever uses the raw event's arrival as the "something moved"
// trigger; it never reads or computes a position at all.
//
// **Unverified on real hardware** — no Linux desktop machine in this
// project's current dev loop, the same caveat this project's other
// Linux platform files already carry.
//
// Needs libX11 + libXi (XInput2) as system development packages — see
// core-lib/CMakeLists.txt's own comment on find_package(X11) for why
// these come from the system rather than vcpkg on Linux.
class MouseMoveHookLinux final : public IMouseMoveHook {
public:
    MouseMoveHookLinux();
    ~MouseMoveHookLinux() override;

    void setCallback(MouseMovedCallback callback) override;
    bool start() override;
    void stop() override;

private:
    MouseMovedCallback m_callback;
    std::thread m_thread;

    // Self-pipe used only to interrupt the blocking poll() this thread
    // waits in — this is what lets stop() wake it cleanly, the X11/Xlib
    // equivalent of Windows' message-loop quit()/PostThreadMessage or
    // macOS's CFRunLoopStop(): none of these three platforms share a
    // single shutdown primitive, so each implementation owns whatever
    // its own native loop needs (see IMouseMoveHook.h's own comment).
    int m_wakeupPipe[2] = {-1, -1};
};

} // namespace lancue::platform::linux_
