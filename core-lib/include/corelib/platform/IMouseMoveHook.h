#pragma once

#include <functional>
#include <memory>

namespace lancue::platform {

// Abstract, event-driven notification that the OS cursor has moved —
// one implementation per OS (§4.4), selected at build time via
// createMouseMoveHook() below, no #ifdef at call sites. Same shape as
// IKeyboardLayoutWatcher.h (setCallback()/start()/stop() + a factory)
// for the same reason that one has: consistency across this project's
// platform interfaces. Windows uses a WH_MOUSE_LL low-level mouse hook
// (the same class of mechanism as WH_KEYBOARD_LL, already proven in
// this project — see keyboard_layout_watcher_windows.h); macOS/Linux
// land their own real, event-driven mechanisms in the same phase this
// interface is introduced (CGEventTap and XInput2 XI_RawMotion
// respectively — see each platform's own .cpp) rather than shipping as
// stubs, per §4.12's cross-platform parity policy, which took effect
// before this interface existed. Unlike IKeyboardLayoutWatcher's own
// macOS/Linux files, these are real implementations from day one, just
// not yet hardware-verified there — see each one's own header comment.
//
// Deliberately carries no position of its own — not in the callback,
// and there is no currentPosition()-style query either (contrast with
// IKeyboardLayoutWatcher::currentLayout()). Every platform's raw
// coordinate space is genuinely different (WH_MOUSE_LL's MSLLHOOKSTRUCT
// gives physical pixels; CGEventGetLocation() gives CoreGraphics global
// points; XInput2 raw motion carries no position at all — see
// mouse_move_hook_linux.cpp's own comment), and none of those is
// guaranteed to line up with Qt's own per-monitor device-independent-
// pixel space that every window in this app is already positioned in
// (ToastWindow's own move()/setScreen() calls, throughout
// toast_window.cpp). QCursor::pos() is already this codebase's one
// authoritative source for "where is the cursor, in the coordinate
// space I can actually use" — ToastFeature::resolveTargetScreens()'s
// own CursorScreen case already reads it for exactly this reason. So
// rather than have three platform implementations each reimplement
// "get the position" and risk three subtly different answers, this
// interface's only job is telling callers *that* the cursor moved;
// callers call QCursor::pos() themselves once notified (see
// MouseWatcherFeature's own comment for where that happens).
//
// The registered callback is always invoked on the thread that called
// start() (in practice, always the main/Qt thread — see main.cpp) —
// never on whatever internal OS-callback thread a given platform
// implementation happens to use internally. Each implementation is
// responsible for its own thread/run-loop management *and* for
// marshaling back via QMetaObject::invokeMethod(qApp, ...,
// Qt::QueuedConnection) before ever invoking the callback — mirrors the
// reasoning in LayoutWatcherFeature::start()'s own comment on why a
// hand-off is needed at all (the OS callback fires nested inside
// whatever dispatch loop the hook is installed on, never safe to treat
// as "the main thread" by default). Here that hand-off responsibility
// is encapsulated inside each platform's own start(), rather than
// pushed up into a shared external thread wrapper the way Phase 2's
// LayoutHookThread does for IKeyboardLayoutWatcher: Windows still needs
// an *externally* pumped message loop (WH_MOUSE_LL's own documented
// contract), but macOS needs a CFRunLoopRun() and Linux needs a
// poll()-based wait on its own X11 connection — three genuinely
// different native pumping mechanisms that a single shared thread
// wrapper can't cleanly express for all three at once. See each
// platform's own .cpp for its specific mechanism.
class IMouseMoveHook {
public:
    using MouseMovedCallback = std::function<void()>;

    virtual ~IMouseMoveHook() = default;

    // Registers the callback invoked every time the OS reports the
    // cursor has actually moved. Must be called before start().
    // Implementations must only ever invoke this from their own
    // OS-callback/hook handler — never from a polling loop (§4.7).
    virtual void setCallback(MouseMovedCallback callback) = 0;

    // Starts observing OS-level cursor-movement notifications.
    // Synchronous: returns only once this platform's mechanism has
    // either been fully installed or has failed to install — never
    // starts a background attempt that might still fail later. Returns
    // false if this platform's mechanism failed to initialize (e.g. a
    // permission not yet granted on macOS, no X11 available on Linux) —
    // callers must log this rather than silently no-op (§4.5).
    virtual bool start() = 0;

    // Synchronous teardown: blocks until every thread/resource this
    // implementation owns has actually stopped. Safe to call even if
    // start() was never called or already failed.
    virtual void stop() = 0;
};

// One-per-OS constructor (§4.4). Implemented in the platform/<os>/ .cpp
// for whichever OS this binary is built for; CMake compiles exactly one
// of those implementation files into core-lib per platform.
std::unique_ptr<IMouseMoveHook> createMouseMoveHook();

} // namespace lancue::platform
