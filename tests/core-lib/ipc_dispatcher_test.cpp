#include <catch2/catch_test_macros.hpp>

#include "corelib/ipc/dispatcher.h"

using lancue::ipc::Dispatcher;
using lancue::ipc::Message;

TEST_CASE("Dispatcher routes to the registered handler", "[ipc]") {
    Dispatcher dispatcher;
    dispatcher.registerHandler("Ping", [](const Message&, Dispatcher::ReplyCallback reply) {
        Message message;
        message.type = "Pong";
        reply(std::move(message));
    });

    Message request;
    request.type = "Ping";

    Message reply;
    dispatcher.dispatch(request, [&reply](Message r) { reply = std::move(r); });

    CHECK(reply.type == "Pong");
}

TEST_CASE("Dispatcher returns an Error message for unknown types", "[ipc]") {
    Dispatcher dispatcher;

    Message request;
    request.type = "SomethingUnregistered";

    Message reply;
    dispatcher.dispatch(request, [&reply](Message r) { reply = std::move(r); });

    CHECK(reply.type == "Error");
    CHECK(reply.payload.contains("reason"));
}

TEST_CASE("Dispatcher supports a handler that defers its reply", "[ipc]") {
    // Exercises the actual reason this contract is callback-based rather
    // than a synchronous return value (see dispatcher.h's own comment):
    // a handler is allowed to not call `reply` at all until some later
    // point, with no nested event loop involved on either side.
    Dispatcher dispatcher;
    Dispatcher::ReplyCallback storedReply;
    dispatcher.registerHandler("Deferred", [&storedReply](const Message&, Dispatcher::ReplyCallback reply) {
        storedReply = std::move(reply);
        // Deliberately not calling storedReply() yet.
    });

    Message request;
    request.type = "Deferred";

    bool replied = false;
    Message reply;
    dispatcher.dispatch(request, [&replied, &reply](Message r) {
        replied = true;
        reply = std::move(r);
    });

    // dispatch() must have returned already without the handler having
    // replied — this is the whole point of the async contract.
    CHECK_FALSE(replied);
    REQUIRE(storedReply);

    Message deferredMessage;
    deferredMessage.type = "DeferredAck";
    storedReply(deferredMessage);

    CHECK(replied);
    CHECK(reply.type == "DeferredAck");
}
