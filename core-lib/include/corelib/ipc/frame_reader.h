#pragma once

#include <string>
#include <vector>

#include "corelib/ipc/message.h"

namespace lancue::ipc {

// Incrementally reassembles length-prefixed frames (see frameMessage) from
// a byte stream that can arrive split across arbitrarily many readyRead()
// calls. One instance per socket/connection — it is not thread-safe and
// not meant to be shared.
class FrameReader {
public:
    // Feeds newly-arrived bytes in. Returns every complete message that
    // could be fully reassembled from what's been fed so far (usually
    // zero or one, but a burst of small writes coalescing into one
    // readyRead() can yield more than one).
    std::vector<Message> feed(const std::string& bytes);

private:
    std::string m_buffer;
};

} // namespace lancue::ipc
