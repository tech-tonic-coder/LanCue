#include <catch2/catch_test_macros.hpp>

#include <QEventLoop>
#include <QTimer>

#include "corelib/platform/selection_clipboard_bridge.h"

using lancue::platform::IClipboardController;
using lancue::platform::IInputSimulator;
using lancue::platform::SelectionClipboardBridge;

namespace {

// A fake IClipboardController standing in for the real OS-backed
// implementations (§4.4's whole point: callers only ever depend on the
// interface). Unlike the real Windows implementation, this fake fires its
// "changed" notification synchronously and immediately rather than
// through an actual OS event — good enough to exercise
// SelectionClipboardBridge's own orchestration logic (the thing this
// phase actually wrote), without needing a real clipboard or a real
// window to test against, the same tradeoff hotkey_combo_test.cpp already
// makes for HotkeyCombo's OS-agnostic pieces.
class FakeClipboardController final : public IClipboardController {
public:
    void setCallback(ClipboardChangedCallback callback) override {
        m_callback = callback;
        if (callback) {
            m_lastCallback = std::move(callback);
        }
    }

    bool start() override {
        m_started = true;
        return m_startResult;
    }

    void stop() override { m_started = false; }

    bool hasText() const override { return m_hasText; }
    std::optional<QString> readText() const override {
        if (m_failNextRead) {
            m_failNextRead = false;
            return std::nullopt;
        }
        return m_text;
    }

    bool writeText(const QString& text) override {
        if (m_failNextWrite) {
            m_failNextWrite = false;
            return false;
        }
        m_text = text;
        m_hasText = true;
        m_writeHistory.append(text);
        // Mirrors the real OS behavior this phase's own header comment
        // relies on: a successful write fires the change notification
        // regardless of whether the new content differs from the old, so
        // a fake that skipped this wouldn't actually exercise
        // captureSelection()'s "notification, not text comparison"
        // design decision at all.
        if (m_started && m_callback) {
            m_callback();
        }
        return true;
    }

    // Test-only controls, not part of IClipboardController.
    void setStartResult(bool result) { m_startResult = result; }
    bool wasStarted() const { return m_started; }
    const QStringList& writeHistory() const { return m_writeHistory; }
    // Makes the *next* readText()/writeText() call fail once (as if
    // OpenClipboard was denied even after retries on real Windows), then
    // reverts to normal behavior — for exercising the failure paths this
    // phase's bug fix added, without needing a real OS-level clipboard
    // lock to reproduce.
    void failNextRead() { m_failNextRead = true; }
    void failNextWrite() { m_failNextWrite = true; }

    // Captures whatever callback was most recently set, independently of
    // m_callback itself later being cleared via setCallback(nullptr) —
    // mirrors how, on real Windows, the listener window keeps existing
    // (and can still have a message dispatched to it) for a brief moment
    // even after SelectionClipboardBridge has told the controller
    // "forget the callback" and moved on. Lets a test simulate a
    // notification arriving late, after the bridge call that registered
    // it has already returned.
    ClipboardChangedCallback lastCallback() const { return m_lastCallback; }

private:
    ClipboardChangedCallback m_callback;
    ClipboardChangedCallback m_lastCallback;
    bool m_started = false;
    bool m_startResult = true;
    bool m_hasText = false;
    QString m_text;
    QStringList m_writeHistory;
    mutable bool m_failNextRead = false;
    bool m_failNextWrite = false;
};

class FakeInputSimulator final : public IInputSimulator {
public:
    void simulateCopy() override {
        ++m_copyCount;
        if (m_copyBehavior) {
            m_copyBehavior();
        }
    }

    void simulatePaste() override { ++m_pasteCount; }

    int copyCount() const { return m_copyCount; }
    int pasteCount() const { return m_pasteCount; }

    // Lets a test decide what "the target application actually copied
    // something" looks like, e.g. writing new text onto the fake
    // clipboard — captureSelection() itself never calls the clipboard
    // directly except through the notification callback, so this is how
    // tests simulate "a real app responded to Ctrl+C".
    void setCopyBehavior(std::function<void()> behavior) { m_copyBehavior = std::move(behavior); }

private:
    int m_copyCount = 0;
    int m_pasteCount = 0;
    std::function<void()> m_copyBehavior;
};

} // namespace

TEST_CASE("SelectionClipboardBridge::captureSelection returns the newly-copied text", "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.writeText(QStringLiteral("previous clipboard contents"));

    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("selected text")); });

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(200);

    CHECK(result.success);
    CHECK(result.text == QStringLiteral("selected text"));
    CHECK(inputSim.copyCount() == 1);
}

TEST_CASE("SelectionClipboardBridge::captureSelection times out when nothing was selected", "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    // No copy behavior wired up: simulateCopy() does nothing to the
    // clipboard, matching "the user had nothing selected".

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(50);

    CHECK_FALSE(result.success);
    CHECK(result.text.isEmpty());
}

TEST_CASE("SelectionClipboardBridge::captureSelection succeeds even when the selection matches the existing "
          "clipboard text",
          "[clipboard]") {
    // The real-world edge case this phase's header comment documents:
    // a naive before/after text comparison would misreport this as "no
    // selection" even though the copy genuinely happened.
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.writeText(QStringLiteral("same text"));
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("same text")); });

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(200);

    CHECK(result.success);
    CHECK(result.text == QStringLiteral("same text"));
}

TEST_CASE("SelectionClipboardBridge::captureSelection fails immediately if the controller can't start",
          "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.setStartResult(false);

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(200);

    CHECK_FALSE(result.success);
    // The copy chord must never fire if the listener couldn't even be
    // installed — there would be nothing to observe the resulting change.
    CHECK(inputSim.copyCount() == 0);
}

TEST_CASE("SelectionClipboardBridge::pasteReplacement writes the replacement, pastes, then restores the original",
          "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.writeText(QStringLiteral("original clipboard contents"));
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("selected text")); });

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto captured = bridge.captureSelectionBlocking(200);
    REQUIRE(captured.success);

    bridge.pasteReplacementBlocking(QStringLiteral("converted text"), 20);

    CHECK(inputSim.pasteCount() == 1);
    // Order matters: the replacement must have been on the clipboard at
    // the moment simulatePaste() was called (second-to-last write), with
    // the original restored only afterward (last write) — a real target
    // application reads whatever's on the clipboard when it processes
    // the paste, not whatever ends up there eventually.
    const auto& history = clipboard.writeHistory();
    REQUIRE(history.size() >= 2);
    CHECK(history.at(history.size() - 2) == QStringLiteral("converted text"));
    CHECK(history.last() == QStringLiteral("original clipboard contents"));
}

TEST_CASE("SelectionClipboardBridge::cancel restores the captured clipboard without pasting", "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.writeText(QStringLiteral("original"));

    SelectionClipboardBridge bridge(clipboard, inputSim);
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("selected")); });
    const auto captured = bridge.captureSelectionBlocking(200);
    REQUIRE(captured.success);

    bridge.cancel();

    CHECK(inputSim.pasteCount() == 0);
    CHECK(clipboard.readText() == std::make_optional(QStringLiteral("original")));
}

TEST_CASE("SelectionClipboardBridge::pasteReplacement without a prior capture pastes but restores nothing",
          "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;

    SelectionClipboardBridge bridge(clipboard, inputSim);
    bridge.pasteReplacementBlocking(QStringLiteral("fixed text"), 10);

    CHECK(inputSim.pasteCount() == 1);
    CHECK(clipboard.readText() == std::make_optional(QStringLiteral("fixed text")));
}

TEST_CASE("SelectionClipboardBridge::captureSelectionBlocking guards against a stale clipboard-changed "
          "notification arriving after it has already returned",
          "[clipboard]") {
    // Regression test for a real crash found on Windows hardware (see
    // this phase's Learnings log): the OS can deliver a
    // WM_CLIPBOARDUPDATE notification for a listener window slightly
    // after a capture has already timed out and its caller has moved on.
    // The first fix (a shared_ptr<bool> guard around the old nested-loop
    // design's dangling `loop`/`changed` references) patched the
    // symptom; SelectionClipboardBridge was then restructured so the
    // callback captures only `this` (long-lived, owned by main.cpp for
    // the daemon's whole life) instead — this test now exercises that
    // restructured design: a late callback invocation must reach
    // finishCapture()'s m_captureInFlight guard and safely no-op, not
    // touch anything from a call that has already completed.
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    // No copy behavior wired up, so this exercises the timeout path —
    // the same path the real crash was caught on.

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(20);
    CHECK_FALSE(result.success);

    const auto lateCallback = clipboard.lastCallback();
    REQUIRE(lateCallback);
    CHECK_NOTHROW(lateCallback());
}

TEST_CASE("SelectionClipboardBridge::captureSelection (async) invokes its completion exactly once", "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("async selection")); });

    SelectionClipboardBridge bridge(clipboard, inputSim);
    int completionCount = 0;
    SelectionClipboardBridge::CaptureResult captured;
    bool completed = false;
    QEventLoop loop;
    bridge.captureSelection(200, [&completionCount, &captured, &completed,
                                   &loop](SelectionClipboardBridge::CaptureResult r) {
        ++completionCount;
        captured = r;
        completed = true;
        loop.quit();
    });
    // The fake clipboard fires its change notification synchronously, so
    // the completion above may already have run before this line — see
    // SelectionClipboardBridge::captureSelectionBlocking()'s own comment
    // for why calling loop.exec() unconditionally here would hang in
    // that case.
    if (!completed) {
        loop.exec();
    }

    CHECK(completionCount == 1);
    CHECK(captured.success);
    CHECK(captured.text == QStringLiteral("async selection"));
}

TEST_CASE("SelectionClipboardBridge::captureSelection rejects a call while one is already in flight",
          "[clipboard]") {
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    // No copy behavior: the first capture stays in flight (nothing ever
    // completes it) for the duration of this test.

    SelectionClipboardBridge bridge(clipboard, inputSim);
    bridge.captureSelection(5000, [](SelectionClipboardBridge::CaptureResult) {});

    bool secondCompleted = false;
    SelectionClipboardBridge::CaptureResult secondResult;
    bridge.captureSelection(5000, [&secondCompleted, &secondResult](SelectionClipboardBridge::CaptureResult r) {
        secondCompleted = true;
        secondResult = r;
    });

    // The second call must be rejected synchronously/immediately, not
    // left pending behind the first.
    CHECK(secondCompleted);
    CHECK_FALSE(secondResult.success);
}

TEST_CASE("SelectionClipboardBridge::captureSelection reports failure, not an empty success, when the clipboard "
          "changed but couldn't be read back",
          "[clipboard]") {
    // Regression test for a real data-loss bug found on Mahdi's own
    // machine (2026-08-27): a transient OpenClipboard failure on the
    // *read-back* step (after the clipboard genuinely changed, i.e. the
    // simulated copy worked) used to be silently reported as a
    // successful capture of an empty string — which a caller would then
    // "convert" (empty to empty) and paste over the user's still-selected
    // real selection, deleting it.
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("selected text")); });
    clipboard.failNextRead();

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto result = bridge.captureSelectionBlocking(200);

    CHECK_FALSE(result.success);
    CHECK(result.text.isEmpty());
}

TEST_CASE("SelectionClipboardBridge::pasteReplacement does not simulate a paste when writeText fails",
          "[clipboard]") {
    // Regression test for the mirror-image bug: a failed *write* used to
    // still trigger simulatePaste() unconditionally, pasting whatever was
    // already on the clipboard (stale/unconverted content) over the
    // user's still-selected text.
    FakeClipboardController clipboard;
    FakeInputSimulator inputSim;
    clipboard.writeText(QStringLiteral("original clipboard contents"));
    inputSim.setCopyBehavior([&clipboard]() { clipboard.writeText(QStringLiteral("selected text")); });

    SelectionClipboardBridge bridge(clipboard, inputSim);
    const auto captured = bridge.captureSelectionBlocking(200);
    REQUIRE(captured.success);

    clipboard.failNextWrite();
    bridge.pasteReplacementBlocking(QStringLiteral("converted text"), 20);

    CHECK(inputSim.pasteCount() == 0);
}

