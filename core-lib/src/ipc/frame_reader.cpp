#include "corelib/ipc/frame_reader.h"

#include <cstdint>

namespace lancue::ipc {

std::vector<Message> FrameReader::feed(const std::string& bytes) {
    m_buffer += bytes;

    std::vector<Message> messages;
    for (;;) {
        if (m_buffer.size() < 4) {
            break;
        }
        const uint32_t length =
            (static_cast<uint8_t>(m_buffer[0]) << 24) |
            (static_cast<uint8_t>(m_buffer[1]) << 16) |
            (static_cast<uint8_t>(m_buffer[2]) << 8) |
            static_cast<uint8_t>(m_buffer[3]);

        if (m_buffer.size() < 4 + length) {
            break; // rest of the frame hasn't arrived yet
        }

        const std::string body = m_buffer.substr(4, length);
        m_buffer.erase(0, 4 + length);

        if (auto message = Message::fromJson(body)) {
            messages.push_back(std::move(*message));
        }
        // A frame that fails to parse is dropped (and should be logged by
        // the caller, which has the connection context) rather than
        // treated as a fatal stream error — one bad message shouldn't take
        // the whole connection down.
    }
    return messages;
}

} // namespace lancue::ipc
