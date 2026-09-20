#include "corelib/ipc/message.h"

#include <cstdint>

namespace lancue::ipc {

std::optional<Message> Message::fromJson(const std::string& jsonText) {
    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(jsonText);
    } catch (const nlohmann::json::parse_error&) {
        return std::nullopt;
    }

    if (!parsed.is_object() || !parsed.contains("type") || !parsed["type"].is_string()) {
        return std::nullopt;
    }

    Message message;
    message.version = parsed.value("version", kProtocolVersion);
    message.type = parsed["type"].get<std::string>();
    message.payload = parsed.value("payload", nlohmann::json::object());
    return message;
}

std::string Message::toJson() const {
    nlohmann::json j;
    j["version"] = version;
    j["type"] = type;
    j["payload"] = payload;
    return j.dump();
}

std::string frameMessage(const Message& message) {
    const std::string body = message.toJson();
    const uint32_t length = static_cast<uint32_t>(body.size());

    std::string frame;
    frame.resize(4);
    frame[0] = static_cast<char>((length >> 24) & 0xFF);
    frame[1] = static_cast<char>((length >> 16) & 0xFF);
    frame[2] = static_cast<char>((length >> 8) & 0xFF);
    frame[3] = static_cast<char>(length & 0xFF);
    frame += body;
    return frame;
}

} // namespace lancue::ipc
