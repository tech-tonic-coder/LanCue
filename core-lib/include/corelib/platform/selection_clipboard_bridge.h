#pragma once

#include <functional>

#include <QString>
#include <QTimer>

#include "corelib/platform/IClipboardController.h"
#include "corelib/platform/IInputSimulator.h"

namespace lancue::platform {

// Phase 5's actual deliverable: "a function that grabs the user's current
// selection from any application and can paste replacement text back into
// it." OS-agnostic — sits alongside hotkey_combo.cpp and
// layout_change_debouncer.cpp as logic that composes the I* interfaces
// rather than implementing one itself (§4.4), so this exact retry/restore
// behavior is written once and reused unchanged by every platform's
// controller/simulator pair, and by Phase 6's conversion engine without
// re-solving the clipboard race condition itself, per this phase's own
// requirements.
//
// Both captureSelection() and pasteReplacement() are event-driven/async —
// neither runs a nested QEventLoop internally, and this is now a settled
// design decision, not a preference: an earlier version of this class ran
// a nested QEventLoop inside captureSelection() itself, and this phase's
// own Learnings log documents two real Windows failures that traced back
// to it — a dangling-reference crash, and (after that was fixed) IpcServer
// connections closing mid-request. That second failure turned out to
// match a documented Qt limitation exactly: reentering the event loop (or
// calling any waitFor*() function) from inside a slot connected to
// QLocalSocket::readyRead() is called out by the Qt community as
// unsupported ("the signal will not be re-emitted" for the duration), and
// Qt's own docs for QAbstractSocket::waitForBytesWritten() state plainly
// that it "may fail randomly on Windows" and recommend "the event loop
// and the bytesWritten() signal" instead — precisely the pattern
// IpcServer::onReadyRead() is. Both methods below are now genuinely
// async: they arm a QTimer and/or register an IClipboardController
// callback and return immediately; whatever they were waiting for
// completes later, from the *same* top-level event loop IpcServer's own
// signal handling already runs on, never a nested one. captureSelectionBlocking()
// exists only as a test convenience (see its own comment) and must never
// be called from anywhere in IpcServer's dispatch chain.
//
// All in-flight-capture state (m_captureTimeoutTimer, m_captureCompletion,
// m_captureInFlight below) lives on *this* SelectionClipboardBridge,
// which main.cpp owns for the daemon's entire lifetime — so the
// OS-facing clipboard-changed callback only ever touches a `this` that's
// always valid, guarded by m_captureInFlight so a late/duplicate
// notification is a deliberate no-op rather than undefined behavior,
// regardless of exact timing.
class SelectionClipboardBridge {
public:
    struct CaptureResult {
        bool success = false;
        QString text;
    };

    using CaptureCompletion = std::function<void(CaptureResult)>;

    // Constants left as compile-time defaults per §4.5 (defined once,
    // referenced everywhere) rather than re-guessed at each call site.
    // Neither is currently exposed as a user-facing setting — nothing in
    // the roadmap asks for that, so they aren't in settings::Settings
    // either (§4.3).
    static constexpr int kDefaultCaptureTimeoutMs = 500;
    static constexpr int kDefaultRestoreDelayMs = 150;

    SelectionClipboardBridge(IClipboardController& clipboard, IInputSimulator& inputSimulator);

    // Saves the clipboard's current contents (restored later by
    // pasteReplacement() or cancel()), simulates the copy chord, and
    // waits for the clipboard to actually change — driven by
    // IClipboardController's own OS-level change-notification callback
    // (event-driven, §4.7), with `timeoutMs` only as a failsafe bound for
    // when no notification ever arrives (most commonly: nothing was
    // actually selected in the focused application, so Ctrl+C had
    // nothing to copy). `onComplete` is invoked exactly once — never
    // zero times, never twice — from this object's own event handling
    // (finishCapture(), below), whether that turns out to be the
    // clipboard notification or the timeout. Never runs a nested event
    // loop — see the class comment above.
    //
    // Relying on the change *notification* rather than comparing the
    // clipboard's text before and after matters for a real edge case: if
    // the current selection happens to be textually identical to
    // whatever's already on the clipboard, a naive "did the text change"
    // comparison would treat that as "nothing was selected" even though
    // the copy genuinely succeeded. Windows' AddClipboardFormatListener
    // fires WM_CLIPBOARDUPDATE on every successful SetClipboardData call
    // regardless of whether the new contents differ from the old, which
    // is exactly the behavior this needs.
    //
    // Calling this again while a capture is already in flight is a
    // programming error (this class supports one capture at a time, by
    // design — nothing in this project's scope needs concurrent
    // captures); `onComplete` is invoked immediately with success=false
    // rather than corrupting the in-flight one.
    void captureSelection(int timeoutMs, CaptureCompletion onComplete);

    // Test-only blocking convenience wrapper around captureSelection()
    // above. MUST NOT be called from anywhere in IpcServer's dispatch
    // chain (or from within any other slot connected to a QLocalSocket
    // signal) — see the class comment for exactly why that's unsafe on
    // Windows. Spins a small local QEventLoop that quits once the async
    // completion fires, tracking completion explicitly rather than
    // assuming a `QEventLoop::quit()` issued before `exec()` has any
    // effect on that `exec()` call (it doesn't — `exec()` resets its own
    // exit state on entry — this bit this phase during its own test
    // development; see the Learnings log).
    CaptureResult captureSelectionBlocking(int timeoutMs = kDefaultCaptureTimeoutMs);

    // Writes `text` to the clipboard, simulates the paste chord, and
    // restores whatever captureSelection() saved — after `restoreDelayMs`
    // has elapsed, not immediately, then invokes `onComplete` (if given).
    // There is no OS callback for "the target application has finished
    // reading the clipboard after processing the simulated paste", so a
    // fixed delay is this phase's own documented, deliberate exception to
    // §4.7's "no polling" rule (a single-shot wait tied to a real,
    // bounded duration, not a repeating timer simulating an event
    // stream) rather than racing the restore against a paste that hasn't
    // actually completed yet. Implemented with a plain
    // `QTimer::singleShot()` functor overload (no context object) that
    // captures only `this` and `onComplete` by value — no nested event
    // loop, no stack-local captured by reference, safe to call from
    // IpcServer's dispatch chain directly.
    //
    // If the clipboard write itself fails (IClipboardController::
    // writeText() returning false — see that method's own doc comment
    // for when this happens on Windows), this does NOT simulate a paste:
    // pasting at that point would replace the user's still-selected text
    // with whatever the clipboard already held (typically the unconverted
    // original, from captureSelection()'s own copy step), which is worse
    // than leaving the selection untouched. `onComplete` still fires in
    // this case. A prior version of this method wrote to the clipboard
    // and simulated the paste unconditionally, without checking whether
    // the write even succeeded — a real data-loss bug found on Mahdi's
    // own machine (2026-08-27): a transient clipboard-locked failure
    // during the write caused the user's selection to be replaced with
    // empty/stale content instead of the intended conversion.
    //
    // Must be called after a captureSelection()/captureSelectionBlocking()
    // that returned success=true; calling it beforehand restores nothing
    // afterward (nothing was saved) but still performs the write/paste —
    // callers are not required to have a prior successful capture (e.g.
    // pasting fixed text with no capture step), only that restoration
    // silently does nothing in that case rather than failing loudly,
    // since "nothing to restore" is this class's/its caller's normal
    // steady state before the first successful capture.
    void pasteReplacement(const QString& text, int restoreDelayMs = kDefaultRestoreDelayMs,
                           std::function<void()> onComplete = {});

    // Test-only blocking convenience wrapper around pasteReplacement()
    // above — same "must not be called from IpcServer's dispatch chain"
    // caveat as captureSelectionBlocking().
    void pasteReplacementBlocking(const QString& text, int restoreDelayMs = kDefaultRestoreDelayMs);

    // Restores the clipboard captureSelection() saved without pasting
    // anything — for a caller (Phase 6) that captured a selection but
    // then decided not to proceed (e.g. nothing convertible was found).
    // Same "nothing to restore" no-op as pasteReplacement() above if
    // captureSelection() was never called or didn't succeed. Purely
    // synchronous (a single writeText() call) — nothing to wait on.
    void cancel();

private:
    // Invoked exactly once per captureSelection() call — either from
    // m_captureTimeoutTimer's own timeout signal (owned by *this*, so
    // Qt's usual safe-disconnect-on-destruction guarantees apply) or from
    // the lambda captureSelection() hands to IClipboardController
    // (`[this]() { finishCapture(true); }` — captures only `this`, never
    // any of captureSelection()'s own locals, which is the whole fix).
    // m_captureInFlight is what makes a second, late firing of either
    // source a safe no-op instead of touching anything twice.
    void finishCapture(bool changed);

    IClipboardController& m_clipboard;
    IInputSimulator& m_inputSimulator;
    QString m_savedClipboardText;
    bool m_hasSavedClipboard = false;

    // A real member, not a per-call QTimer::singleShot — reused across
    // every captureSelection() call rather than constructed fresh each
    // time, since it now needs to be started/stopped from finishCapture()
    // regardless of which of its two trigger sources fired first.
    QTimer m_captureTimeoutTimer;
    CaptureCompletion m_captureCompletion;
    bool m_captureInFlight = false;
};

} // namespace lancue::platform
