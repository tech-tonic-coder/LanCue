#pragma once

#include <Windows.h>

#include "corelib/platform/IKeyboardLayoutWatcher.h"

namespace lancue::platform::windows {

// Low-level-keyboard-hook-based implementation. Replaces an earlier TSF
// (ITfInputProcessorProfileActivationSink / ITfActiveLanguageProfileNotifySink)
// attempt that failed identically (CONNECT_E_CANNOTCONNECT) across four
// independent fixes on the real dev machine — see the Phase 2 Learnings
// entries for the full history. TSF is Microsoft's documented modern
// mechanism for this, but proved too fragile to get working from a
// headless daemon in practice; WH_KEYBOARD_LL is the pragmatic
// alternative several real-world layout-switcher utilities use instead.
//
// Why a low-level hook and not WM_INPUTLANGCHANGE: that message is only
// posted to whichever window has keyboard focus, and this daemon owns no
// window. WH_KEYBOARD_LL is system-wide and focus-independent — it sees
// every keystroke before any window does, which is also why it must be
// used narrowly: this class only ever inspects a key's virtual-key code
// to decide whether a modifier was just released, and immediately
// discards everything else. No key, character, or text content is ever
// stored or logged.
//
// Why not RegisterHotKey instead: RegisterHotKey claims a specific,
// unclaimed combination for this process alone. The OS's own
// layout-switch combination (Alt+Shift, Win+Space, etc., whatever the
// user has configured) is already claimed by the input subsystem itself
// — it can't be registered by anyone else. RegisterHotKey is the right
// tool for a later phase's own application-defined hotkeys (§3's planned
// IGlobalHotkeyManager), where LanCue owns the combination; it's the
// wrong tool here, where it doesn't.
//
// Needs no window and no COM/TSF at all: WH_KEYBOARD_LL only requires the
// installing thread to be pumping messages, which QCoreApplication::exec()
// already does.
//
// Phase 7 revision (2026-08-30): WH_KEYBOARD_LL alone only ever reacts to
// a modifier key being *released* — a reasonable proxy for "the user just
// used a keyboard shortcut to switch layouts," but blind to every other
// way Windows can change the active layout: selecting one from the
// taskbar's language flyout (a mouse action, no modifier key involved at
// all), or Windows' own "remember a different layout per app window"
// feature silently switching the active layout as a side effect of
// focus moving to a different window (also no modifier key involved,
// especially when that focus change itself was mouse-driven). A second,
// independent OS notification — SetWinEventHook(EVENT_SYSTEM_FOREGROUND),
// confirmed via research to require no DLL injection when installed with
// WINEVENT_OUTOFCONTEXT (unlike a *global* WH_SHELL hook, which needs a
// real injected DLL) — is registered alongside the keyboard hook to catch
// the "switched to a different window with its own remembered layout"
// case.
//
// Phase 7 revision (2026-08-31, round 7): EVENT_SYSTEM_FOREGROUND does
// not fire for the taskbar language-flyout case after all (confirmed on
// real hardware — selecting a layout from the tray flyout produces no
// foreground-window round-trip; a subsequent modifier-key press is what
// was actually surfacing the already-changed layout via the keyboard
// hook, not the flyout selection itself). A third, independent trigger —
// RegisterShellHookWindow()'s HSHELL_LANGUAGE notification — closes this
// gap. Unlike SetWindowsHookEx(WH_SHELL, ...) with a system-wide
// (dwThreadId=0) scope, RegisterShellHookWindow needs no DLL injection at
// all: it asks the shell to deliver notifications, as an ordinary
// broadcast window message, to one specific top-level window this class
// owns — confirmed via research (2026-08-31) that these are delivered as
// broadcast messages, which is why the window this class creates for
// this purpose must be a real top-level window, not a message-only one
// (HWND_MESSAGE-parented windows never receive broadcast messages, per
// Microsoft's own documented behavior). All three triggers funnel into
// the same deferred re-check (scheduleLayoutRecheck()) and the same
// settle-delay reasoning (see that method's own comment).
//
// Phase 7 revision (2026-09-04, round 8): confirmed on real hardware
// that HSHELL_LANGUAGE *also* does not fire for the taskbar flyout case
// (most likely because the modern Windows 10/11 input flyout switches
// layouts through TSF rather than the legacy path HSHELL_LANGUAGE is
// tied to) — round 7's fix did not close the gap it was built for.
// TSF itself remains off the table (see the Phase 2 Learnings entry:
// four independent, differently-shaped attempts all failed identically
// with CONNECT_E_CANNOTCONNECT; do not retry it). A fourth trigger —
// `EVENT_OBJECT_NAMECHANGE`, delivered through the same `SetWinEventHook`
// mechanism already proven reliable in this file for
// `EVENT_SYSTEM_FOREGROUND` — is registered for this instead: it is the
// same MSAA notification screen readers rely on to announce a language
// change, so it is a plausible fit, but **this one is genuinely
// unverified** — unlike the first three triggers, no real-hardware
// confirmation exists yet that it actually fires for this case. Two
// safety measures accompany it specifically because it's unverified and,
// if it does fire, fires extremely often system-wide (it's one of the
// most common MSAA events in the OS — almost any UI text change anywhere
// triggers it): (1) `winEventProc()` only acts on it when the event's own
// hwnd is rooted under the taskbar's top-level window (`Shell_TrayWnd`/
// `Shell_SecondaryTrayWnd` — stable class names since the Windows XP era,
// unlike the taskbar's *internal* control class names, which have
// changed across Windows versions and can't be relied on); (2)
// `scheduleLayoutRecheck()` itself now dedupes against the
// last-*notified* layout before invoking the callback, so even a
// spurious match (wrong hwnd slipping past the taskbar-root filter, or
// the filter itself proving too loose) degrades to a wasted
// `GetKeyboardLayout()` query rather than a visibly wrong toast — this
// dedup applies to all four triggers, not just this one, and is a
// worthwhile hardening on its own regardless of how this one trigger
// pans out.
class KeyboardLayoutWatcherWindows final : public IKeyboardLayoutWatcher {
public:
    KeyboardLayoutWatcherWindows();
    ~KeyboardLayoutWatcherWindows() override;

    void setCallback(LayoutChangedCallback callback) override;
    bool start() override;
    void stop() override;
    QString currentLayout() const override;

private:
    static LRESULT CALLBACK lowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam);
    // Registered separately from the keyboard hook (§ below) — fires on
    // *any* foreground-window change, system-wide, regardless of whether
    // a keyboard shortcut was involved. This is what catches "switch
    // between windows that each remember their own layout." Confirmed on
    // real hardware (round 7) to *not* catch the taskbar language flyout
    // — see shellHookWndProc() for the trigger that does. Neither this
    // nor the flyout case involves releasing a modifier key the way a
    // keyboard-driven layout switch does.
    static void CALLBACK winEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                       DWORD idEventThread, DWORD dwmsEventTime);
    // WndProc for the real (invisible) top-level window this class owns
    // purely to receive RegisterShellHookWindow()'s HSHELL_LANGUAGE
    // broadcast — see the class-level comment (round 7) for why this
    // exists and why the window can't be message-only. Reacts only to
    // the dynamically-registered "SHELLHOOK" message with
    // wParam == HSHELL_LANGUAGE; everything else is passed to
    // DefWindowProcW unchanged.
    static LRESULT CALLBACK shellHookWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    // Cheap, version-stable pre-filter for the round-8 NAMECHANGE trigger
    // (see the class-level comment): true only if hwnd's top-level
    // ancestor is the taskbar itself (Shell_TrayWnd on the primary
    // monitor, Shell_SecondaryTrayWnd on others). A single GetAncestor +
    // GetClassNameW call, no process handle opened — deliberately not
    // filtering by any *inner* control class name, since those are
    // undocumented and have changed across Windows versions, unlike the
    // taskbar's own root class.
    static bool isTaskbarWindow(HWND hwnd);
    // Shared by all four triggers above — see this method's own comment
    // (in the .cpp) for why the actual currentLayout() query is deferred
    // rather than immediate, regardless of which trigger scheduled it.
    void scheduleLayoutRecheck();

    // SetWindowsHookEx's callback is a plain function pointer with no
    // room for a `this` argument, so the single active instance is
    // tracked here — fine in practice since only one
    // KeyboardLayoutWatcherWindows is ever constructed per process
    // (createKeyboardLayoutWatcher() is called once, by LayoutWatcherFeature).
    static KeyboardLayoutWatcherWindows* s_activeInstance;

    LayoutChangedCallback m_callback;
    HHOOK m_keyboardHook = nullptr;
    HWINEVENTHOOK m_foregroundEventHook = nullptr;
    // Round 8: same WinEvent mechanism as m_foregroundEventHook above, a
    // second independent hook handle because SetWinEventHook only covers
    // one contiguous [eventMin,eventMax] range per call and
    // EVENT_OBJECT_NAMECHANGE isn't adjacent to EVENT_SYSTEM_FOREGROUND.
    HWINEVENTHOOK m_nameChangeEventHook = nullptr;
    // Real (never-shown) top-level window + the message id RegisterShell-
    // HookWindow()'s notifications arrive as — see shellHookWndProc()'s
    // own comment. Both are only ever touched on the dedicated hook
    // thread (created in start(), destroyed in stop()).
    HWND m_shellHookWindow = nullptr;
    UINT m_shellHookMessageId = 0;
    // Round 8 dedup safety net — see scheduleLayoutRecheck()'s own
    // comment. Empty until the first LayoutChanged notification actually
    // goes out; only ever touched on the dedicated hook thread.
    QString m_lastNotifiedLayout;
};

} // namespace lancue::platform::windows
