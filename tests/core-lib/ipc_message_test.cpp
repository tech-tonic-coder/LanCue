#include <catch2/catch_test_macros.hpp>

#include "corelib/ipc/message.h"

using lancue::ipc::frameMessage;
using lancue::ipc::Message;

TEST_CASE("Message round-trips through JSON", "[ipc]") {
    Message original;
    original.type = "Ping";
    original.payload = {{"foo", 42}};

    const std::string json = original.toJson();
    const auto parsed = Message::fromJson(json);

    REQUIRE(parsed.has_value());
    CHECK(parsed->type == "Ping");
    CHECK(parsed->payload.at("foo") == 42);
    CHECK(parsed->version == lancue::ipc::kProtocolVersion);
}

TEST_CASE("Message::fromJson rejects malformed input", "[ipc]") {
    CHECK_FALSE(Message::fromJson("not json").has_value());
    CHECK_FALSE(Message::fromJson("{}").has_value());
    CHECK_FALSE(Message::fromJson(R"({"type": 5})").has_value());
}

TEST_CASE("frameMessage prefixes a big-endian length", "[ipc]") {
    Message m;
    m.type = "Ping";
    const std::string frame = frameMessage(m);
    const std::string body = m.toJson();

    REQUIRE(frame.size() == body.size() + 4);
    const uint32_t length = (static_cast<uint8_t>(frame[0]) << 24) |
                             (static_cast<uint8_t>(frame[1]) << 16) |
                             (static_cast<uint8_t>(frame[2]) << 8) |
                             static_cast<uint8_t>(frame[3]);
    CHECK(length == body.size());
}
