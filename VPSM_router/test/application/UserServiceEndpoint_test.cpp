#include "../../src/application/ControlPlane/UserServiceEndpoint.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::UserServiceEndpoint;

    class UserServiceFake final : public vpsm::server::port::IUserService {
    public:
        std::optional<std::uint64_t> createPeer(const std::string& nickname, const std::string& passwordHash) override {
            lastNickname = nickname;
            lastPassword = passwordHash;
            return createPeerResult;
        }
        bool deletePeer(std::uint64_t) override { return false; }
        std::optional<std::uint64_t> createNetwork(std::uint64_t, const std::string&, const std::string&) override { return std::nullopt; }
        bool deleteNetwork(std::uint64_t, std::uint64_t) override { return false; }
        std::optional<std::uint32_t> joinNetwork(std::uint64_t, std::uint64_t, const std::string&) override { return std::nullopt; }
        bool leaveNetwork(std::uint64_t, std::uint64_t) override { return false; }

        std::optional<std::uint64_t> createPeerResult = 42;
        std::string lastNickname;
        std::string lastPassword;
    };

    std::vector<std::uint8_t> bytes(const std::string& s) {
        return std::vector<std::uint8_t>(s.begin(), s.end());
    }

    TEST(UserServiceEndpointTest, createPeer_ValidJson_Returns200True) {
        UserServiceFake service;
        UserServiceEndpoint endpoint(service, UserServiceEndpoint::Operation::CREATE_PEER);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/create-peer",
            .headers = {{"Content-Type", "application/json"}},
            .body = bytes(R"({"nickname":"n","passwordHash":"p"})"),
        });

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(response.contentType, "application/json");
        EXPECT_EQ(service.lastNickname, "n");
        EXPECT_EQ(service.lastPassword, "p");
    }

    TEST(UserServiceEndpointTest, createPeer_WrongContentType_Returns415True) {
        UserServiceFake service;
        UserServiceEndpoint endpoint(service, UserServiceEndpoint::Operation::CREATE_PEER);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/create-peer",
            .headers = {{"Content-Type", "text/plain"}},
            .body = bytes("x"),
        });

        EXPECT_EQ(response.status, 415);
    }

    TEST(UserServiceEndpointTest, createPeer_InvalidJson_Returns400True) {
        UserServiceFake service;
        UserServiceEndpoint endpoint(service, UserServiceEndpoint::Operation::CREATE_PEER);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/create-peer",
            .headers = {{"Content-Type", "application/json"}},
            .body = bytes("{broken"),
        });

        EXPECT_EQ(response.status, 400);
    }
}
