#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace lancue::ipc {

// Wire format version for the Message envelope itself (not per-message-type
// versioning — each message type's payload can evolve independently within
// this envelope). Bump this only for a breaking change to the envelope
// shape below (e.g. renaming "type"/"version"/"payload"). See the roadmap's
// Phase 1 Learnings entry for why a single flat version field was chosen
// over per-field versioning.
inline constexpr int kProtocolVersion = 1;

// Every message exchanged between lancue-core and lancue-settings uses this
// envelope. `type` selects the handler in Dispatcher; `payload` is
// type-specific and left as a raw json object so each handler owns its own
// (de)serialization rather than Message knowing about every message type
// that will ever exist (new message type = new handler file, not a change
// here — see roadmap §4.4).
struct Message {
    int version = kProtocolVersion;
    std::string type;
    nlohmann::json payload = nlohmann::json::object();

    // Returns std::nullopt on malformed JSON or a missing/non-string "type"
    // field, rather than throwing — callers (IpcServer/IpcClient) are
    // reading untrusted bytes off a socket and should treat a parse
    // failure as "drop this message and log it", not a crash.
    static std::optional<Message> fromJson(const std::string& jsonText);

    std::string toJson() const;
};

// Frames a message for the wire: a 4-byte big-endian length prefix followed
// by the UTF-8 JSON payload. QLocalSocket is a byte stream, not a message
// stream, so a length prefix is what lets the reading side know where one
// message ends and the next begins — without it, two messages written back
// to back could arrive as a single readyRead() with no self-evident split
// point.
std::string frameMessage(const Message& message);

} // namespace lancue::ipc
