#include "corelib/platform/selection_clipboard_bridge.h"

#include <QEventLoop>

#include "corelib/logging/logger.h"

namespace lancue::platform {

SelectionClipboardBridge::SelectionClipboardBridge(IClipboardController& clipboard, IInputSimulator& inputSimulator)
    : m_clipboard(clipboard), m_inputSimulator(inputSimulator) {
    m_captureTimeoutTimer.setSingleShot(true);
    // Context object is m_captureTimeoutTimer itself (a real member, not
    // a per-call temporary) — its lifetime exactly matches *this*, so
    // this connection is inherently safe for as long as the bridge
    // itself exists, with no per-call setup/teardown needed. The "timed
    // out" log belongs here rather than inside finishCapture() itself:
    // finishCapture(false) is also reached when m_clipboard.start() fails
    // outright (see captureSelection() below), which already logs its
    // own specific reason — logging "timed out" there too would be
    // misleading for a failure that was never actually a timeout.
    QObject::connect(&m_captureTimeoutTimer, &QTimer::timeout, &m_captureTimeoutTimer, [this]() {
        logWarning(QStringLiteral("SelectionClipboardBridge: timed out waiting for a clipboard change after "
                                   "simulated copy — likely nothing was selected."));
        finishCapture(false);
    });
}

void SelectionClipboardBridge::captureSelection(int timeoutMs, CaptureCompletion onComplete) {
    if (m_captureInFlight) {
        logError(QStringLiteral(
            "SelectionClipboardBridge: captureSelection() called while another capture is already in "
            "flight; ignoring this call."));
        if (onComplete) {
            onComplete(CaptureResult{});
        }
        return;
    }

    m_hasSavedClipboard = m_clipboard.hasText();
    if (m_hasSavedClipboard) {
        const std::optional<QString> saved = m_clipboard.readText();
        if (saved.has_value()) {
            m_savedClipboardText = saved.value();
        } else {
            // Couldn't read the clipboard's prior contents to save them —
            // treat as "nothing to restore" rather than proceeding with a
            // stale/empty m_savedClipboardText that pasteReplacement()/
            // cancel() would later write back as if it were real saved
            // content. Doesn't block the capture itself from continuing;
            // only the restore step is affected.
            logWarning(QStringLiteral("SelectionClipboardBridge: could not read the clipboard's prior contents to "
                                       "save them; nothing will be restored after this operation."));
            m_hasSavedClipboard = false;
            m_savedClipboardText = QString();
        }
    } else {
        m_savedClipboardText = QString();
    }

    m_captureCompletion = std::move(onComplete);
    m_captureInFlight = true;

    // Captures only `this` — never captureSelection()'s own locals (there
    // are none held past this point) — so this callback is safe to fire
    // at any time for as long as this SelectionClipboardBridge exists,
    // which main.cpp guarantees for the daemon's whole lifetime. See the
    // class's own header comment for the real crash this replaces the
    // fix for.
    m_clipboard.setCallback([this]() { finishCapture(true); });

    if (!m_clipboard.start()) {
        logError(QStringLiteral("SelectionClipboardBridge: clipboard controller failed to start; cannot capture."));
        finishCapture(false);
        return;
    }

    m_inputSimulator.simulateCopy();

    // simulateCopy() can trigger the clipboard-changed callback
    // synchronously/reentrant — this project's own test fakes do exactly
    // that, and it's not impossible for a real OS to deliver the
    // notification within the same call either. If that already ran
    // finishCapture() to completion (m_captureInFlight is false again by
    // now), starting the timeout timer here would be pointless — it
    // would just fire later into finishCapture()'s own no-op guard.
    if (m_captureInFlight) {
        // Failsafe bound, not the primary wait mechanism — see this
        // method's own header comment on why the change notification,
        // not this timer, is what normally ends the wait.
        m_captureTimeoutTimer.start(timeoutMs);
    }
}

void SelectionClipboardBridge::finishCapture(bool changed) {
    if (!m_captureInFlight) {
        // A second/late firing of either trigger source (the timer, or a
        // clipboard notification the OS had already queued before
        // m_clipboard.stop() below actually tore the listener down) —
        // deliberately a no-op. `this` is always valid to check here
        // regardless of when this fires; that's the whole point of
        // keeping this state on the object instead of a call's locals.
        return;
    }

    m_captureInFlight = false;
    m_captureTimeoutTimer.stop();
    m_clipboard.stop();
    m_clipboard.setCallback(nullptr);

    CaptureResult result;
    if (changed) {
        const std::optional<QString> text = m_clipboard.readText();
        if (text.has_value()) {
            result.success = true;
            result.text = text.value();
        } else {
            // The clipboard DID change (simulateCopy() presumably worked,
            // and the OS notification fired) but the contents couldn't
            // actually be read back — this must NOT be reported as a
            // successful capture of empty text. A prior version of this
            // method did exactly that, and the resulting empty
            // "converted" text got pasted over the user's still-selected
            // real selection, deleting it — a real bug found on Mahdi's
            // own machine (2026-08-27). This is a genuine failure, not
            // the "nothing was selected" case the timeout path below
            // represents, so it's logged distinctly.
            logError(QStringLiteral("SelectionClipboardBridge: clipboard changed but could not be read back; "
                                     "treating this capture as failed."));
        }
    }
    // No generic "failed" log for the changed=false/timeout case here —
    // the timer's own timeout log (this class's constructor) already
    // covers it, as does m_clipboard.start()'s own failure log in
    // captureSelection() above.

    // Moved out and cleared before invoking: if `onComplete` itself calls
    // captureSelection() again (Phase 6 chaining another operation), that
    // re-entrant call must see m_captureCompletion already empty and
    // m_captureInFlight already false, not still holding this now-stale
    // completion.
    auto completion = std::move(m_captureCompletion);
    m_captureCompletion = nullptr;
    if (completion) {
        completion(result);
    }
}

SelectionClipboardBridge::CaptureResult SelectionClipboardBridge::captureSelectionBlocking(int timeoutMs) {
    QEventLoop loop;
    CaptureResult result;
    bool completed = false;
    captureSelection(timeoutMs, [&loop, &result, &completed](CaptureResult r) {
        result = r;
        completed = true;
        loop.quit();
    });

    // captureSelection() can complete entirely synchronously/reentrant —
    // e.g. IClipboardController::start() failing outright, or (in the
    // fakes this project's own tests use) a clipboard-changed callback
    // that fires immediately rather than from a genuine later OS event.
    // When that happens, the completion lambda above already ran and
    // called loop.quit() *before* loop.exec() below has even started —
    // and QEventLoop::exec() resets its own exit state each time it's
    // called, so a quit() issued before the first exec() call has no
    // effect on that call: calling loop.exec() unconditionally here would
    // hang until m_captureTimeoutTimer's own failsafe eventually fired
    // finishCapture() a second time, except finishCapture() is
    // deliberately a no-op on that second firing (m_captureInFlight is
    // already false) — so nothing would ever wake this loop up at all.
    // Checking `completed` first sidesteps the whole "quit before exec"
    // question rather than relying on Qt's exact semantics for it.
    if (!completed) {
        loop.exec();
    }
    return result;
}

void SelectionClipboardBridge::pasteReplacement(const QString& text, int restoreDelayMs,
                                                 std::function<void()> onComplete) {
    if (!m_clipboard.writeText(text)) {
        // Do NOT simulate a paste here: writeText() failing means the
        // clipboard was left exactly as it was before this call (never
        // partially written or emptied — see IClipboardController::
        // writeText()'s own contract) — simulating Ctrl+V now would paste
        // whatever was already there (typically the just-captured,
        // unconverted original selection) over the user's still-selected
        // text, which is worse than doing nothing. A prior version of
        // this method proceeded to paste regardless of whether the write
        // even succeeded, which caused real data loss on Mahdi's own
        // machine (2026-08-27): a transient clipboard-locked write
        // failure resulted in the user's selection being replaced with
        // stale/empty content.
        logError(QStringLiteral("SelectionClipboardBridge: writeText failed; not pasting, to avoid replacing the "
                                 "selection with unconverted or stale clipboard content."));
        if (m_hasSavedClipboard) {
            if (!m_clipboard.writeText(m_savedClipboardText)) {
                logWarning(QStringLiteral("SelectionClipboardBridge: could not restore the original clipboard "
                                           "contents after a failed write."));
            }
            m_hasSavedClipboard = false;
        }
        if (onComplete) {
            onComplete();
        }
        return;
    }

    m_inputSimulator.simulatePaste();

    // Functor overload with no context object: this timer isn't tied to
    // any receiver's lifetime, so nothing here needs guarding against a
    // destroyed receiver the way m_captureTimeoutTimer (a real member,
    // connected with m_captureTimeoutTimer itself as context) does — the
    // lambda captures `this` (long-lived, owned by main.cpp) and
    // `onComplete` by value, never anything short-lived by reference. No
    // nested event loop — see the class comment on why that matters.
    QTimer::singleShot(restoreDelayMs, [this, onComplete = std::move(onComplete)]() {
        if (m_hasSavedClipboard) {
            if (!m_clipboard.writeText(m_savedClipboardText)) {
                logWarning(QStringLiteral("SelectionClipboardBridge: could not restore the original clipboard "
                                           "contents after pasting."));
            }
            m_hasSavedClipboard = false;
        }
        if (onComplete) {
            onComplete();
        }
    });
}

void SelectionClipboardBridge::pasteReplacementBlocking(const QString& text, int restoreDelayMs) {
    QEventLoop loop;
    bool completed = false;
    pasteReplacement(text, restoreDelayMs, [&loop, &completed]() {
        completed = true;
        loop.quit();
    });
    // Same reasoning as captureSelectionBlocking(): this check used to be
    // defensive/future-proofing only, since pasteReplacement() previously
    // never completed synchronously — but as of the writeText()-failure
    // fix (see pasteReplacement()'s own doc comment), it now can: a
    // failed write invokes `onComplete` immediately, before this line
    // even runs, rather than through QTimer::singleShot(). This check is
    // what makes that path safe to block on too, not just the normal one.
    if (!completed) {
        loop.exec();
    }
}

void SelectionClipboardBridge::cancel() {
    if (m_hasSavedClipboard) {
        if (!m_clipboard.writeText(m_savedClipboardText)) {
            logWarning(
                QStringLiteral("SelectionClipboardBridge: could not restore the original clipboard contents "
                                "on cancel()."));
        }
        m_hasSavedClipboard = false;
    }
}

} // namespace lancue::platform
