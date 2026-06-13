#include "core/Config.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("resource byte caps stay aligned across bindings") {
    REQUIRE(luax::kMaxWebResponseBytes == luax::kMaxFsReadBytes);
    REQUIRE(luax::kMaxWebResponseBytes == 32 * 1024 * 1024);
    REQUIRE(luax::kMaxWebRequestBytes == luax::kMaxFsWriteBytes);
    REQUIRE(luax::kMaxWebRequestBytes == 32 * 1024 * 1024);
}

TEST_CASE("filesystem read and write caps are symmetric") {
    REQUIRE(luax::kMaxFsWriteBytes == luax::kMaxFsReadBytes);
    REQUIRE(luax::kMaxFsWriteBytes == 32 * 1024 * 1024);
}

TEST_CASE("websocket byte caps stay aligned and below web limits") {
    REQUIRE(luax::kMaxWebSocketSendBytes == luax::kMaxWebSocketReceiveBytes);
    REQUIRE(luax::kMaxWebSocketSendBytes == 8 * 1024 * 1024);
    REQUIRE(luax::kMaxWebSocketSendBytes < luax::kMaxWebRequestBytes);
    REQUIRE(luax::kMaxWebSocketReceiveBytes < luax::kMaxWebResponseBytes);
}

TEST_CASE("websocket connection caps stay fixed") {
    REQUIRE(luax::kMaxWebSocketConnections == 16);
    REQUIRE(luax::kMaxWebSocketServers == 2);
    REQUIRE(luax::kMaxWebSocketServerClients == 32);
}
