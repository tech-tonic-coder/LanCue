#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <QString>

namespace lancue::platform {

// Abstract, event-driven clipboard access — one implementation per OS
// (§4.4), selected at build time via createClipboardController() below,
// no #ifdef at call sites. Text-only for now: nothing in this project's
// current scope (converting selected text between keyboard layouts) reads
// or writes any other clipboard format, so this interface doesn't grow a
// QMimeData-style generic payload until a real requirement asks for one
// (§4.3).
//
// Unlike IKeyboardLayoutWatcher/IGlobalHotkeyManager, whose start()/stop()
// bracket the whole daemon's lifetime, this interface's start()/stop()
// are meant to bracket one short "wait for the next clipboard change"
// window at a time (see SelectionClipboardBridge, this phase's actual
// caller) — set the callback, start(), simulate a copy, stop() once that
// one change has been observed or timed out. Cheap and idempotent enough
// on every OS's real mechanism (Windows: AddClipboardFormatListener/
// RemoveClipboardFormatListener) to be started and stopped repeatedly
// like this rather than left running for the process's whole life.
class IClipboardController {
public:
    // Invoked once, from this platform's own OS-callback/notification
    // handler (never a polling loop, §4.7), the next time the clipboard's
    // contents change after start(). Fires exactly once per start()/
    // stop() bracket — callers needing to observe more than one change
    // call start() again after handling the first.
    using ClipboardChangedCallback = std::function<void()>;

    virtual ~IClipboardController() = default;

    // Must be called before start().
    virtual void setCallback(ClipboardChangedCallback callback) = 0;

    // Starts observing OS-level clipboard-change notifications. Returns
    // false if this platform's mechanism failed to initialize — callers
    // must log this rather than silently no-op (§4.5).
    virtual bool start() = 0;

    virtual void stop() = 0;

    // True if the clipboard currently holds text this interface can read
    // (as opposed to being empty or holding some other format entirely,
    // e.g. an image) — SelectionClipboardBridge uses this before saving
    // the clipboard's prior contents, so it doesn't later "restore" text
    // over something that was never text to begin with.
    virtual bool hasText() const = 0;

    // Best-effort synchronous read of the clipboard's current text.
    // Returns an empty-but-present QString if the clipboard genuinely
    // holds no text (check hasText() first to distinguish that from a
    // read that failed outright). Returns std::nullopt only when the
    // read itself could not be performed at all — e.g. on Windows,
    // OpenClipboard denied even after this implementation's own retry
    // attempts, typically because another process (a clipboard manager,
    // Windows' own Clipboard History, cloud clipboard sync, or an
    // antivirus/DLP hook) is transiently holding the clipboard open.
    // Callers must NOT treat std::nullopt the same as an empty string —
    // conflating "couldn't read" with "read, and it was empty" is what
    // caused a real data-loss bug (see SelectionClipboardBridge's own
    // handling of this return type): a failed read was previously
    // silently reported as a successful empty capture, which then got
    // "converted" (empty to empty) and pasted, replacing a real,
    // still-selected selection with nothing.
    virtual std::optional<QString> readText() const = 0;

    // Replaces the clipboard's contents with `text`. Returns false if the
    // write could not be performed at all (e.g. Windows: OpenClipboard
    // denied even after retries), in which case the clipboard is left
    // exactly as it was before this call — never partially written or
    // emptied. Callers must not proceed to simulate a paste when this
    // returns false; see SelectionClipboardBridge::pasteReplacement()'s
    // own handling for why (the same data-loss bug this class's readText()
    // doc comment describes had a mirror-image write-side cause too).
    // Does not itself wait for or fire ClipboardChangedCallback for this
    // write — that callback is this interface's own OS notification
    // doing its job for changes from *any* source (including this call),
    // not a synchronous confirmation hook.
    virtual bool writeText(const QString& text) = 0;
};

// One-per-OS constructor (§4.4). Implemented in the platform/<os>/ .cpp
// for whichever OS this binary is built for; CMake compiles exactly one
// of those implementation files into core-lib per platform.
std::unique_ptr<IClipboardController> createClipboardController();

} // namespace lancue::platform
