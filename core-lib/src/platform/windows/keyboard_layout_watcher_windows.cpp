#include "keyboard_layout_watcher_windows.h"

#include <cwchar>
#include <iterator>

#include <QTimer>

#include "corelib/logging/logger.h"

namespace lancue::platform::windows {

namespace {

QString normalizeLangId(LANGID langid) {
    const LCID lcid = MAKELCID(langid, SORT_DEFAULT);
    wchar_t buffer[LOCALE_NAME_MAX_LENGTH] = {};
    const int len = LCIDToLocaleName(lcid, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    if (len <= 0) {
        // Empty, not the literal string "unknown" — see currentLayout()'s
        // own comment for why an empty QString is the sentinel this
        // implementation returns for "couldn't determine the layout",
        // and IKeyboardLayoutWatcher.h's own doc comment for how callers
        // (i18n::layoutDisplayName()) are expected to handle it.
        return QString();
    }
    // LCIDToLocaleName returns e.g. "en-US" — lowercased to match the
    // normalized scheme documented in IKeyboardLayoutWatcher.h.
    return QString::fromWCharArray(buffer).toLower();
}

bool isModifierVirtualKey(DWORD vkCode) {
    switch (vkCode) {
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_LWIN:
        case VK_RWIN:
            return true;
        default:
            return false;
    }
}

// A real, documented Windows race, not a guess: GetKeyboardLayout()'s own
// Microsoft documentation says a caller that "caches" layout info should
// rely on WM_INPUTLANGCHANGE to know when to re-read it — implying a
// reactive re-query triggered by some other signal (either trigger below)
// isn't guaranteed to already see the committed value at the exact
// instant that signal fires. Multiple independent real-world reports of
// exactly this symptom exist (e.g. an AutoHotkey Community thread,
// 2026-08-30 research: "my script shows the previous layout ID and only
// on the second try of the hotkey will it show the current layout id").
// This delay is what scheduleLayoutRecheck() below waits before actually
// querying — see that method's own comment for why a delay, not a
// blocking Sleep, and why this specific value.
constexpr int kLayoutSettleDelayMs = 50;

// A real top-level window is required here — see the header's round-7
// comment for why a message-only (HWND_MESSAGE-parented) window would
// silently miss RegisterShellHookWindow()'s notifications. It's never
// shown (no WS_VISIBLE) and never interacted with; its only job is to
// exist as a valid message target and dispatch its own WndProc — nothing
// else references its class name, so a project-unique literal is enough
// to avoid clashing with any other window class in this process.
constexpr wchar_t kShellHookWindowClassName[] = L"LanCueShellHookWindow";

} // namespace

KeyboardLayoutWatcherWindows* KeyboardLayoutWatcherWindows::s_activeInstance = nullptr;

KeyboardLayoutWatcherWindows::KeyboardLayoutWatcherWindows() = default;

KeyboardLayoutWatcherWindows::~KeyboardLayoutWatcherWindows() {
    stop();
}

void KeyboardLayoutWatcherWindows::setCallback(LayoutChangedCallback callback) {
    m_callback = std::move(callback);
}

bool KeyboardLayoutWatcherWindows::start() {
    if (s_activeInstance) {
        // Shouldn't happen in practice (see the header comment on
        // s_activeInstance), but fail loudly rather than silently
        // clobbering an existing hook if it ever does.
        lancue::logError(QStringLiteral(
            "KeyboardLayoutWatcherWindows: start() called while another instance's hook is already active."));
        return false;
    }

    m_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardLayoutWatcherWindows::lowLevelKeyboardProc,
                                        GetModuleHandleW(nullptr), 0);
    if (!m_keyboardHook) {
        lancue::logError(
            QStringLiteral("KeyboardLayoutWatcherWindows: SetWindowsHookExW(WH_KEYBOARD_LL) failed (error=%1).")
                .arg(GetLastError()));
        return false;
    }

    // Best-effort, not fatal if it fails: the keyboard hook above is the
    // primary, always-required mechanism (this class's own Deliverable);
    // this one only adds coverage for layout changes that never touch a
    // modifier key at all (§ this class's own header comment). A failure
    // here (should be rare — WINEVENT_OUTOFCONTEXT needs no special
    // privilege) is logged but doesn't fail start() the way the keyboard
    // hook's own failure does.
    m_foregroundEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                                             &KeyboardLayoutWatcherWindows::winEventProc, 0, 0,
                                             WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (!m_foregroundEventHook) {
        lancue::logError(QStringLiteral("KeyboardLayoutWatcherWindows: SetWinEventHook(EVENT_SYSTEM_FOREGROUND) "
                                         "failed; layout changes triggered by switching windows will not be "
                                         "detected."));
    }

    // Also best-effort, same reasoning as the WinEvent hook above: this is
    // the trigger specifically for the taskbar language-flyout case
    // (round 7 — EVENT_SYSTEM_FOREGROUND alone doesn't cover it; see the
    // header's own comment). WNDCLASSEXW is registered once per process
    // and left registered for the process's lifetime, matching how every
    // other Win32 window-class registration in this codebase behaves;
    // ERROR_CLASS_ALREADY_EXISTS on a second start() (shouldn't happen
    // given s_activeInstance's own singleton guard above, but harmless if
    // it ever did) is not treated as a failure.
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.lpfnWndProc = &KeyboardLayoutWatcherWindows::shellHookWndProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kShellHookWindowClassName;
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        lancue::logError(QStringLiteral("KeyboardLayoutWatcherWindows: RegisterClassExW for the shell-hook window "
                                         "failed (error=%1); the taskbar language flyout will not be detected.")
                              .arg(GetLastError()));
    } else {
        // A real, never-shown top-level window (no WS_VISIBLE, no
        // parent) — see kShellHookWindowClassName's own comment for why
        // it must be a real top-level window rather than a message-only
        // one. WS_EX_TOOLWINDOW is defense-in-depth only (this window is
        // never shown either way): it keeps an invisible utility window
        // like this one out of any taskbar/Alt-Tab/Task-View enumeration
        // that some other process or shell extension might do, the same
        // precaution real shell-hook-consuming apps (taskbar
        // replacements, etc.) take for this exact kind of window.
        m_shellHookWindow = CreateWindowExW(WS_EX_TOOLWINDOW, kShellHookWindowClassName, L"", WS_OVERLAPPED, 0, 0, 0,
                                             0, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!m_shellHookWindow) {
            lancue::logError(QStringLiteral("KeyboardLayoutWatcherWindows: CreateWindowExW for the shell-hook "
                                             "window failed (error=%1); the taskbar language flyout will not be "
                                             "detected.")
                                  .arg(GetLastError()));
        } else {
            m_shellHookMessageId = RegisterWindowMessageW(L"SHELLHOOK");
            if (!m_shellHookMessageId || !RegisterShellHookWindow(m_shellHookWindow)) {
                lancue::logError(
                    QStringLiteral("KeyboardLayoutWatcherWindows: RegisterShellHookWindow failed (error=%1); the "
                                   "taskbar language flyout will not be detected.")
                        .arg(GetLastError()));
                DestroyWindow(m_shellHookWindow);
                m_shellHookWindow = nullptr;
                m_shellHookMessageId = 0;
            }
        }
    }

    // Round 8, also best-effort: EVENT_OBJECT_NAMECHANGE is the fourth,
    // still-unverified trigger for the taskbar language-flyout case (see
    // the header's own comment for why rounds 6 and 7 didn't close this
    // gap, and why this one's hwnd is filtered via isTaskbarWindow()
    // rather than accepted unfiltered).
    m_nameChangeEventHook = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr,
                                             &KeyboardLayoutWatcherWindows::winEventProc, 0, 0,
                                             WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (!m_nameChangeEventHook) {
        lancue::logError(QStringLiteral("KeyboardLayoutWatcherWindows: SetWinEventHook(EVENT_OBJECT_NAMECHANGE) "
                                         "failed; the taskbar language flyout will not be detected."));
    }

    s_activeInstance = this;
    lancue::logInfo(QStringLiteral("KeyboardLayoutWatcherWindows: low-level keyboard hook installed."));
    return true;
}

void KeyboardLayoutWatcherWindows::stop() {
    if (m_shellHookWindow) {
        // DeregisterShellHookWindow before DestroyWindow, not after —
        // mirrors every other unhook-then-release ordering in this
        // method (WinEvent, then keyboard hook): release the OS-facing
        // registration first, then the resource it pointed at.
        DeregisterShellHookWindow(m_shellHookWindow);
        DestroyWindow(m_shellHookWindow);
        m_shellHookWindow = nullptr;
        m_shellHookMessageId = 0;
    }
    if (m_foregroundEventHook) {
        UnhookWinEvent(m_foregroundEventHook);
        m_foregroundEventHook = nullptr;
    }
    if (m_nameChangeEventHook) {
        UnhookWinEvent(m_nameChangeEventHook);
        m_nameChangeEventHook = nullptr;
    }
    if (m_keyboardHook) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
    if (s_activeInstance == this) {
        s_activeInstance = nullptr;
    }
}

QString KeyboardLayoutWatcherWindows::currentLayout() const {
    // Layout is a per-thread property in Win32, so "the current layout"
    // for a headless process with no window of its own means the layout
    // of whichever thread owns the foreground window. Returns an empty
    // QString — not a placeholder id like "unknown-unknown" — when that
    // can't be determined (no foreground window at all, or its locale
    // name can't be resolved), which i18n::layoutDisplayName() (core-lib)
    // recognizes explicitly and turns into a proper localized "layout
    // unknown" message rather than an accidentally-blank toast title.
    // Console/terminal-hosted windows are a known, separate category of
    // Windows quirk here — see this method's own Learnings entry: they
    // don't reliably hit this empty-sentinel path (GetKeyboardLayout()
    // typically still returns *some* validly-formatted HKL for them), so
    // this fallback alone doesn't fully solve "terminals report the
    // wrong layout," only "no foreground window could be found at all."
    HWND foreground = GetForegroundWindow();
    DWORD threadId = 0;
    if (foreground) {
        threadId = GetWindowThreadProcessId(foreground, nullptr);
    }
    const HKL hkl = GetKeyboardLayout(threadId);
    const LANGID langid = LOWORD(reinterpret_cast<UINT_PTR>(hkl));
    return normalizeLangId(langid);
}

void KeyboardLayoutWatcherWindows::scheduleLayoutRecheck() {
    // Deliberately deferred, not queried immediately — see
    // kLayoutSettleDelayMs's own comment for the real, documented Windows
    // race this avoids: GetKeyboardLayout() queried at the exact instant
    // the triggering event fires can still return the *previous* layout,
    // because Windows hasn't necessarily finished propagating the switch
    // to the foreground thread's own input-locale association yet (this
    // is a genuinely different bug from — and was masked by, until Phase
    // 7's toast made staleness user-visible for the first time — the
    // nested-hook-callback delivery-timing issue Phase 7's earlier
    // Learnings entries already cover; fixing that one did not fix this
    // one, since this is about the *value* being stale, not about *when*
    // an already-correct value gets delivered).
    //
    // A blocking Sleep() here instead would be a correctness *and* a
    // system-wide-input-lag problem: WH_KEYBOARD_LL's callback blocks
    // every other application's keyboard input system-wide for as long
    // as it runs (Microsoft's own guidance: hook procedures must return
    // quickly — and winEventProc() should stay well-behaved for the same
    // reason, even though its own documented constraints are looser), so
    // this schedules a QTimer::singleShot() instead and returns
    // immediately — the actual query happens ~50ms later, off the
    // triggering callback's own critical path, on this same thread's own
    // event loop (LayoutWatcherFeature's dedicated hook thread, which
    // already runs exec() — see that feature's own start() comment). No
    // receiver/context object is passed: this class's only instance is a
    // process-lifetime singleton (s_activeInstance) stopped explicitly
    // (both hooks unhooked + the hook thread's own exec() returning)
    // before it could ever be destroyed while a 50ms timer is still
    // pending — see LayoutWatcherFeature::stop()'s own comment for that
    // ordering guarantee.
    //
    // 50ms is an empirical, not a formally-verified, value — grounded in
    // community reports of this exact class of race (2026-08-30 research)
    // being resolved by "trying again shortly after," not in a documented
    // exact number from Microsoft. It adds negligible perceived latency
    // (well under typical human "instant" perception, and small next to
    // LayoutChangeDebouncer's own 120ms window) — but if a future report
    // ever shows a layout still occasionally reading stale on real
    // hardware, this is the value to increase first, not the mechanism to
    // second-guess (see §4.9's own hard gate on this — this comment
    // exists so a future patch attempt starts here instead of relearning
    // the underlying race from scratch).
    QTimer::singleShot(kLayoutSettleDelayMs, [this]() {
        // Round 8 dedup: query fresh, but only actually notify if the
        // layout genuinely differs from the last one this class already
        // reported. Cheap, and load-bearing now that one of the four
        // triggers (EVENT_OBJECT_NAMECHANGE) is unverified and, if its
        // isTaskbarWindow() pre-filter ever proves looser than intended,
        // could otherwise fire this path far more often than an actual
        // layout switch happens — this is what keeps that outcome a
        // wasted query instead of a visibly wrong extra toast (ToastFeature
        // shows one on every LayoutChanged event it receives, with no
        // deduping of its own — see that class's own subscribe() comment).
        const QString layout = currentLayout();
        if (layout == m_lastNotifiedLayout) {
            return;
        }
        m_lastNotifiedLayout = layout;
        if (m_callback) {
            m_callback(layout);
        }
    });
}

LRESULT CALLBACK KeyboardLayoutWatcherWindows::lowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    // Hook procedures must return quickly and must always call
    // CallNextHookEx, regardless of what this one does with the event —
    // skipping it, or taking too long, can get Windows to silently
    // remove the hook. scheduleLayoutRecheck() itself only ever
    // schedules a deferred timer and returns immediately (see that
    // method's own comment) — it never blocks here.
    if (code == HC_ACTION && s_activeInstance) {
        // Only key-up matters: it's the natural trigger point for "the
        // user just finished a modifier chord that might have switched
        // layouts" — not a guarantee the switch has already fully
        // propagated by this exact instant (see scheduleLayoutRecheck()'s
        // own comment on why the actual query is deferred, not immediate).
        // Only the virtual-key code is inspected — no character, scan
        // code, or typed text is ever read or retained here.
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            const auto* keyInfo = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
            if (keyInfo && isModifierVirtualKey(keyInfo->vkCode)) {
                s_activeInstance->scheduleLayoutRecheck();
            }
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void CALLBACK KeyboardLayoutWatcherWindows::winEventProc(HWINEVENTHOOK /*hook*/, DWORD event, HWND hwnd,
                                                           LONG /*idObject*/, LONG /*idChild*/,
                                                           DWORD /*idEventThread*/, DWORD /*dwmsEventTime*/) {
    // Catches: switching focus to a window that Windows' own "remember a
    // different layout per app window" feature associates with a
    // different layout than the one just active. Confirmed on real
    // hardware (round 7) *not* to cover the taskbar language flyout —
    // that selection produces no foreground-window round-trip — which is
    // what the EVENT_OBJECT_NAMECHANGE branch below (round 8) exists for
    // instead. Neither case involves a modifier key at all, so
    // WH_KEYBOARD_LL alone (above) never sees them.
    if (event == EVENT_SYSTEM_FOREGROUND && s_activeInstance) {
        s_activeInstance->scheduleLayoutRecheck();
        return;
    }

    // Round 8, unverified — see the header's own comment. isTaskbarWindow()
    // is the cheap pre-filter that keeps this from reacting to the vast
    // majority of EVENT_OBJECT_NAMECHANGE traffic system-wide (this is
    // one of the most common MSAA events in the OS); scheduleLayoutRecheck()'s
    // own dedup against the last-notified layout is the second, cheaper-
    // to-reason-about safety net if this filter ever proves too loose.
    if (event == EVENT_OBJECT_NAMECHANGE && s_activeInstance && isTaskbarWindow(hwnd)) {
        s_activeInstance->scheduleLayoutRecheck();
    }
}

bool KeyboardLayoutWatcherWindows::isTaskbarWindow(HWND hwnd) {
    if (!hwnd) {
        return false;
    }
    const HWND root = GetAncestor(hwnd, GA_ROOT);
    if (!root) {
        return false;
    }
    wchar_t className[64] = {};
    if (GetClassNameW(root, className, static_cast<int>(std::size(className))) <= 0) {
        return false;
    }
    return wcscmp(className, L"Shell_TrayWnd") == 0 || wcscmp(className, L"Shell_SecondaryTrayWnd") == 0;
}

LRESULT CALLBACK KeyboardLayoutWatcherWindows::shellHookWndProc(HWND hwnd, UINT message, WPARAM wParam,
                                                                  LPARAM lParam) {
    // The "SHELLHOOK" message id is dynamic (assigned per-session by
    // RegisterWindowMessageW, not a fixed constant — see start()'s own
    // comment) — s_activeInstance->m_shellHookMessageId is the value to
    // compare against, resolved once at start() and stable for this
    // instance's whole lifetime. HSHELL_LANGUAGE (wParam) is the shell's
    // own documented notification for "keyboard language was changed or
    // a new keyboard layout was loaded" — this is what actually fires
    // for the taskbar language-flyout selection (round 7), independent
    // of both other triggers above.
    if (s_activeInstance && message == s_activeInstance->m_shellHookMessageId && wParam == HSHELL_LANGUAGE) {
        s_activeInstance->scheduleLayoutRecheck();
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace lancue::platform::windows

namespace lancue::platform {

std::unique_ptr<IKeyboardLayoutWatcher> createKeyboardLayoutWatcher() {
    return std::make_unique<windows::KeyboardLayoutWatcherWindows>();
}

} // namespace lancue::platform

