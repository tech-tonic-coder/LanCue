#pragma once

#include <functional>
#include <memory>

#include <QString>

namespace lancue::platform {

// Abstract, event-driven notification of the user's active keyboard input
// layout — one implementation per OS (§4.4), selected at build time via
// createKeyboardLayoutWatcher() below, no #ifdef at call sites. Windows
// uses a WH_KEYBOARD_LL low-level keyboard hook, re-reading the layout on
// modifier key-up (see keyboard_layout_watcher_windows.cpp for why: an
// earlier TSF-based attempt hit an unresolvable CONNECT_E_CANNOTCONNECT
// in this headless-process context — §4.9's own example of that pivot);
// macOS/Linux implementations land when those platforms are actually
// being built and tested (currently graceful "unsupported" stubs — see
// those .cpp files).
//
// Every id this interface hands back — from the callback or from
// currentLayout() — is already normalized to LanCue's internal scheme: a
// lowercase BCP-47-style language-region tag (e.g. "en-us", "fa-ir").
// Callers never see a raw OS-specific layout name/HKL/input-source id.
class IKeyboardLayoutWatcher {
public:
    using LayoutChangedCallback = std::function<void(const QString& normalizedLayoutId)>;

    virtual ~IKeyboardLayoutWatcher() = default;

    // Registers the callback invoked every time the user's active layout
    // actually changes. Must be called before start(). Implementations
    // must only ever invoke this from their own OS-callback/notification
    // handler — never from a polling loop (§4.7).
    virtual void setCallback(LayoutChangedCallback callback) = 0;

    // Starts observing OS-level layout-change notifications. Returns
    // false if this platform's mechanism failed to initialize (e.g. a
    // COM/TSF failure on Windows, an unimplemented/unavailable mechanism
    // on macOS/Linux) — callers must log this rather than silently
    // no-op (§4.5).
    virtual bool start() = 0;

    virtual void stop() = 0;

    // Best-effort synchronous read of the current layout, in the same
    // normalized scheme as the callback. Cheap and stateless on every
    // implementation (a plain OS query — GetForegroundWindow +
    // GetKeyboardLayout on Windows — not backed by the hook/notification
    // mechanism above). Used once, at startup, to seed initial state
    // before the first real change event fires (LayoutWatcherFeature's
    // own startup seed — see that feature's start() comment) — not a
    // polling mechanism, and not called again afterward. (Phase 7's
    // ToastFeature briefly polled this directly on its own QTimer as an
    // independent layout-change signal, working around a real timing bug
    // in the callback-driven path below — see the Phase 7 Learnings entry
    // for why that was reverted once the actual root cause was fixed:
    // that path is safe to rely on again, and duplicating this query on
    // a second timer was never necessary once it was.)
    virtual QString currentLayout() const = 0;
};

// One-per-OS constructor (§4.4). Implemented in the platform/<os>/ .cpp
// for whichever OS this binary is built for; CMake compiles exactly one
// of those implementation files into core-lib per platform.
std::unique_ptr<IKeyboardLayoutWatcher> createKeyboardLayoutWatcher();

} // namespace lancue::platform
