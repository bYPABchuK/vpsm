#include "../../src/application/ControlPlane/DispatchEchoEndpoint.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::DispatchEchoEndpoint;

    std::vector<std::uint8_t> asBytes(const std::string& text) {
        return std::vector<std::uint8_t>(text.begin(), text.end());
    }

    TEST(DispatchEchoEndpointTest, handle_ValidJson_Returns200JsonResponseTrue) {
        DispatchEchoEndpoint endpoint;

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/dispatch/echo",
            .headers = {{"Content-Type", "application/json"}},
            .body = asBytes(R"({"x":1})"),
        });

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(response.contentType, "application/json");
    }

    TEST(DispatchEchoEndpointTest, handle_InvalidJson_Returns400True) {
        DispatchEchoEndpoint endpoint;

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/dispatch/echo",
            .headers = {{"Content-Type", "application/json"}},
            .body = asBytes("{broken"),
        });

        EXPECT_EQ(response.status, 400);
    }

    TEST(DispatchEchoEndpointTest, handle_WrongContentType_Returns415True) {
        DispatchEchoEndpoint endpoint;

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/dispatch/echo",
            .headers = {{"Content-Type", "text/plain"}},
            .body = asBytes("hi"),
        });

        EXPECT_EQ(response.status, 415);
    }
}
