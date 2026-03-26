#include "../../src/application/ControlPlane/DispatchEchoEndpoint.hpp"
#include "../../src/application/ControlPlane/JsonRequestDecoder.hpp"
#include "../../src/application/ControlPlane/JsonResponseEncoder.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::DispatchEchoEndpoint;

    std::vector<std::uint8_t> asBytes(const std::string& text) {
        return std::vector<std::uint8_t>(text.begin(), text.end());
    }

    TEST(DispatchEchoEndpointTest, handle_ValidJson_Returns200JsonResponseTrue) {
        vpsm::server::application::JsonRequestDecoder decoder;
        vpsm::server::application::JsonResponseEncoder encoder;
        DispatchEchoEndpoint endpoint(decoder, encoder);

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
        vpsm::server::application::JsonRequestDecoder decoder;
        vpsm::server::application::JsonResponseEncoder encoder;
        DispatchEchoEndpoint endpoint(decoder, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/dispatch/echo",
            .headers = {{"Content-Type", "application/json"}},
            .body = asBytes("{broken"),
        });

        EXPECT_EQ(response.status, 400);
    }

    TEST(DispatchEchoEndpointTest, handle_WrongContentType_Returns415True) {
        vpsm::server::application::JsonRequestDecoder decoder;
        vpsm::server::application::JsonResponseEncoder encoder;
        DispatchEchoEndpoint endpoint(decoder, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/dispatch/echo",
            .headers = {{"Content-Type", "text/plain"}},
            .body = asBytes("hi"),
        });

        EXPECT_EQ(response.status, 415);
    }
}
