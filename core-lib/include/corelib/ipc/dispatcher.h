#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "corelib/ipc/message.h"

namespace lancue::ipc {

// Routes an incoming Message to whichever handler registered for its
// `type`, per roadmap §4.4 ("IPC messages → dispatch table"): a new message
// type is a new handler registration, not a new branch in an existing
// switch/if chain here.
//
// Fully async/callback-based, not "return a Message" — every handler is
// handed a ReplyCallback and calls it whenever its reply is ready, rather
// than dispatch() itself returning one. This isn't a style choice: Qt's
// own documentation for QAbstractSocket::waitForBytesWritten() states
// plainly that it "may fail randomly on Windows" and recommends "the
// event loop and the bytesWritten() signal" instead, and the Qt community
// independently documents that reentering the event loop (or calling any
// waitFor*() function) from inside a slot connected to readyRead() means
// that signal "will not be [re-]emitted" for the duration. IpcServer's
// onReadyRead() is exactly such a slot; a handler that ran a nested
// QEventLoop (this project's own earlier attempt, before this fix — see
// Phase 5's Learnings log for the real Windows crash and connection drops
// that produced) is precisely the pattern both sources warn against. A
// handler needing to wait on something (Phase 5's clipboard capture, or
// any future handler with a similar need) instead stores the
// ReplyCallback and invokes it later, from the *same* top-level event
// loop that's already running — never a nested one.
class Dispatcher {
public:
    using ReplyCallback = std::function<void(Message)>;
    using Handler = std::function<void(const Message& request, ReplyCallback reply)>;

    // Registers `handler` for `type`. Re-registering the same type replaces
    // the previous handler — callers are expected to register each type
    // exactly once at startup (see core-lib/src/ipc/handlers/), so a
    // silent replace is fine; nothing in this project relies on multiple
    // handlers per type.
    void registerHandler(const std::string& type, Handler handler);

    // Looks up the handler for `request.type` and invokes it with `reply`.
    // If no handler is registered, calls `reply` immediately with a
    // Message of type "Error" (payload: {"reason": ...}) instead — an
    // unknown message type is an expected runtime possibility (e.g.
    // version skew between core and settings), not a programming error,
    // so this never throws either.
    void dispatch(const Message& request, ReplyCallback reply) const;

private:
    std::unordered_map<std::string, Handler> m_handlers;
};

} // namespace lancue::ipc
