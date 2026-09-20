#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include <thread>

#include "corelib/platform/IMouseMoveHook.h"

namespace lancue::platform::macos {

// CGEventTap-based implementation (kCGEventMouseMoved, listen-only) —
// the standard, Apple-documented mechanism for observing pointer motion
// system-wide without polling (confirmed via research: real-world
// accessibility/automation tools use exactly this API for exactly this
// purpose). Requires the user to grant this app "Input Monitoring"
// permission in System Settings > Privacy & Security — CGEventTapCreate
// itself returns null until that's granted, which start() below
// surfaces as a normal, loggable false return (§4.5), not a crash;
// macOS prompts the user with its own permission dialog the first time
// this runs.
//
// **Unverified on real hardware** — there is no macOS machine in this
// project's current dev loop (the same caveat every other macOS
// platform file in this project already carries). Written and reasoned
// from CGEventTap's own documented contract and confirmed real-world
// usage patterns, not confirmed working end to end yet — flag this to
// Mahdi rather than treat it as equivalent in confidence to the
// Windows implementation.
//
// Same "callback only decides whether to notify, never reads or keeps
// button/click state" scope discipline as the Windows implementation
// (see that file's own comment) — kCGEventTapOptionListenOnly
// additionally guarantees this implementation can't modify or block any
// event even if it wanted to.
class MouseMoveHookMacos final : public IMouseMoveHook {
public:
    MouseMoveHookMacos();
    ~MouseMoveHookMacos() override;

    void setCallback(MouseMovedCallback callback) override;
    bool start() override;
    void stop() override;

private:
    static CGEventRef handleEvent(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon);

    MouseMovedCallback m_callback;
    std::thread m_thread;

    // Set from inside the thread started by start(), before it releases
    // the readiness semaphore start() waits on — safely visible to
    // stop() (running on whatever thread called it) via that same
    // acquire/release handshake, same informal-synchronization style
    // LayoutHookThread already uses in this codebase (no separate mutex
    // needed for a single write followed by a single later read after a
    // semaphore handshake).
    CFRunLoopRef m_runLoop = nullptr;
    CFMachPortRef m_tap = nullptr;
    CFRunLoopSourceRef m_runLoopSource = nullptr;
};

} // namespace lancue::platform::macos
