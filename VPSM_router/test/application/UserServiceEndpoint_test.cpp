#include "../../src/application/ControlPlane/Endpoints/CreatePeerEndpoint.hpp"
#include "../../src/application/ControlPlane/Endpoints/CreateNetworkEndpoint.hpp"
#include "../../src/application/ControlPlane/Endpoints/JoinNetworkEndpoint.hpp"
#include "../../src/application/ControlPlane/Endpoints/LeaveNetworkEndpoint.hpp"
#include "../../src/application/ControlPlane/Endpoints/UserNetworkPeersListEndpoint.hpp"
#include "../../src/application/ControlPlane/JsonCreatePeerDecoder.hpp"
#include "../../src/application/ControlPlane/JsonCreatePeerResponseEncoder.hpp"
#include "../../src/application/ControlPlane/JsonCreateNetworkDecoder.hpp"
#include "../../src/application/ControlPlane/JsonCreateNetworkResponseEncoder.hpp"
#include "../../src/application/ControlPlane/JsonJoinNetworkDecoder.hpp"
#include "../../src/application/ControlPlane/JsonJoinNetworkResponseEncoder.hpp"
#include "../../src/application/ControlPlane/JsonLeaveNetworkDecoder.hpp"
#include "../../src/application/ControlPlane/JsonLeaveNetworkResponseEncoder.hpp"
#include "../../src/application/ControlPlane/JsonUserNetworkPeersListResponseEncoder.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {
    using vpsm::server::application::ControlRequest;
    using vpsm::server::application::endpoints::CreateNetworkEndpoint;
    using vpsm::server::application::endpoints::CreatePeerEndpoint;
    using vpsm::server::application::endpoints::JoinNetworkEndpoint;
    using vpsm::server::application::endpoints::LeaveNetworkEndpoint;
    using vpsm::server::application::endpoints::UserNetworkPeersListEndpoint;

    class UserServiceFake final : public vpsm::server::port::IUserService {
    public:
        std::optional<std::uint64_t> createPeer(const std::string& nickname, const std::string& passwordHash) override {
            lastNickname = nickname;
            lastPassword = passwordHash;
            return createPeerResult;
        }
        bool deletePeer(std::uint64_t) override { return false; }
        bool deleteNetwork(std::uint64_t, std::uint64_t) override { return false; }
        std::optional<std::uint64_t> findPeerIdByNickname(const std::string&) const override { return std::nullopt; }
        bool verifyPeerPassword(std::uint64_t, const std::string&) const override { return true; }

        std::optional<std::uint64_t> createPeerResult = 42;
        std::optional<std::uint64_t> createNetworkResult = 77;
        std::optional<std::uint32_t> joinNetworkResult = 111;
        bool leaveNetworkResult = true;
        std::string lastNickname;
        std::string lastPassword;
        std::uint64_t lastOwnerPeerId = 0;
        std::string lastNetworkName;
        std::uint64_t lastJoinPeerId = 0;
        std::uint64_t lastJoinNetworkId = 0;
        std::uint64_t lastLeavePeerId = 0;
        std::uint64_t lastLeaveNetworkId = 0;

        std::optional<std::uint64_t> createNetwork(std::uint64_t ownerPeerId, const std::string& name, const std::string& passwordHash) override {
            lastOwnerPeerId = ownerPeerId;
            lastNetworkName = name;
            lastPassword = passwordHash;
            return createNetworkResult;
        }

        std::optional<std::uint32_t> joinNetwork(std::uint64_t peerId, std::uint64_t networkId, const std::string& passwordHash) override {
            lastJoinPeerId = peerId;
            lastJoinNetworkId = networkId;
            lastPassword = passwordHash;
            return joinNetworkResult;
        }

        bool leaveNetwork(std::uint64_t peerId, std::uint64_t networkId) override {
            lastLeavePeerId = peerId;
            lastLeaveNetworkId = networkId;
            return leaveNetworkResult;
        }

        mutable std::vector<vpsm::server::domain::VNetwork> userNetworks;
        mutable std::vector<vpsm::server::domain::Peer> networkPeers;

        std::vector<vpsm::server::domain::VNetwork> listUserNetworks(std::uint64_t) const override {
            return userNetworks;
        }
        std::vector<vpsm::server::domain::Peer> listNetworkPeers(std::uint64_t) const override {
            return networkPeers;
        }
    };

    std::vector<std::uint8_t> bytes(const std::string& s) {
        return std::vector<std::uint8_t>(s.begin(), s.end());
    }

    TEST(UserEndpointsTest, createPeer_ValidJson_Returns200True) {
        UserServiceFake service;
        vpsm::server::application::JsonCreatePeerDecoder decoder;
        vpsm::server::application::JsonCreatePeerResponseEncoder encoder;
        CreatePeerEndpoint endpoint(service, decoder, encoder);

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

    TEST(UserEndpointsTest, createPeer_WrongContentType_Returns415True) {
        UserServiceFake service;
        vpsm::server::application::JsonCreatePeerDecoder decoder;
        vpsm::server::application::JsonCreatePeerResponseEncoder encoder;
        CreatePeerEndpoint endpoint(service, decoder, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/create-peer",
            .headers = {{"Content-Type", "text/plain"}},
            .body = bytes("x"),
        });

        EXPECT_EQ(response.status, 400);
    }

    TEST(UserEndpointsTest, createPeer_InvalidJson_Returns400True) {
        UserServiceFake service;
        vpsm::server::application::JsonCreatePeerDecoder decoder;
        vpsm::server::application::JsonCreatePeerResponseEncoder encoder;
        CreatePeerEndpoint endpoint(service, decoder, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/create-peer",
            .headers = {{"Content-Type", "application/json"}},
            .body = bytes("{broken"),
        });

        EXPECT_EQ(response.status, 400);
    }

    TEST(UserEndpointsTest, createNetwork_ValidJson_Returns200True) {
        UserServiceFake service;
        vpsm::server::application::JsonCreateNetworkDecoder decoder;
        vpsm::server::application::JsonCreateNetworkResponseEncoder encoder;
        CreateNetworkEndpoint endpoint(service, decoder, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "POST",
            .path = "/user/networks",
            .headers = {{"Content-Type", "application/json"}},
            .body = bytes(R"({"ownerPeerId":7,"name":"n1","passwordHash":"ph"})"),
            .authenticatedPeerId = 7,
        });

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(service.lastOwnerPeerId, 7u);
        EXPECT_EQ(service.lastNetworkName, "n1");
    }

    TEST(UserEndpointsTest, joinNetwork_FromPathParams_UsesRestParamsTrue) {
        UserServiceFake service;
        vpsm::server::application::JsonJoinNetworkDecoder decoder;
        vpsm::server::application::JsonJoinNetworkResponseEncoder encoder;
        JoinNetworkEndpoint endpoint(service, decoder, encoder);

        ControlRequest req{
            .method = "PUT",
            .path = "/user/networks/9/members/5",
            .headers = {{"Content-Type", "application/json"}},
            .pathParams = {{"networkId", "9"}, {"peerId", "5"}},
            .body = bytes(R"({"passwordHash":"ph"})"),
            .authenticatedPeerId = 5,
        };

        const auto response = endpoint.handle(req);

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(service.lastJoinPeerId, 5u);
        EXPECT_EQ(service.lastJoinNetworkId, 9u);
    }

    TEST(UserEndpointsTest, leaveNetwork_FromPathParams_UsesRestParamsTrue) {
        UserServiceFake service;
        vpsm::server::application::JsonLeaveNetworkDecoder decoder;
        vpsm::server::application::JsonLeaveNetworkResponseEncoder encoder;
        LeaveNetworkEndpoint endpoint(service, decoder, encoder);

        ControlRequest req{
            .method = "DELETE",
            .path = "/user/networks/9/members/5",
            .headers = {{"Content-Type", "application/json"}},
            .pathParams = {{"networkId", "9"}, {"peerId", "5"}},
            .body = bytes("{}"),
            .authenticatedPeerId = 5,
        };

        const auto response = endpoint.handle(req);

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(service.lastLeavePeerId, 5u);
        EXPECT_EQ(service.lastLeaveNetworkId, 9u);
    }

    TEST(UserEndpointsTest, userNetworkPeersList_ValidRequest_ReturnsNetworksWithPeersTrue) {
        UserServiceFake service;
        vpsm::server::application::JsonUserNetworkPeersListResponseEncoder encoder;
        UserNetworkPeersListEndpoint endpoint(service, encoder);

        service.userNetworks.push_back(vpsm::server::domain::VNetwork{
            .id = 9,
            .name = "net-9",
            .owner = vpsm::server::domain::Peer{.peerId = 5, .vip = 0},
        });
        service.networkPeers.push_back(vpsm::server::domain::Peer{.peerId = 5, .vip = 1001});
        service.networkPeers.push_back(vpsm::server::domain::Peer{.peerId = 6, .vip = 1002});

        const auto response = endpoint.handle(ControlRequest{
            .method = "GET",
            .path = "/user/5/network-peers-list",
            .pathParams = {{"id", "5"}},
            .authenticatedPeerId = 5,
        });

        EXPECT_EQ(response.status, 200);
        EXPECT_EQ(response.contentType, "application/json");

        const std::string body(response.body.begin(), response.body.end());
        EXPECT_NE(body.find("\"ok\":true"), std::string::npos);
        EXPECT_NE(body.find("\"networks\""), std::string::npos);
        EXPECT_NE(body.find("\"peers\""), std::string::npos);
    }

    TEST(UserEndpointsTest, userNetworkPeersList_Forbidden_Returns403True) {
        UserServiceFake service;
        vpsm::server::application::JsonUserNetworkPeersListResponseEncoder encoder;
        UserNetworkPeersListEndpoint endpoint(service, encoder);

        const auto response = endpoint.handle(ControlRequest{
            .method = "GET",
            .path = "/user/5/network-peers-list",
            .pathParams = {{"id", "5"}},
            .authenticatedPeerId = 7,
        });

        EXPECT_EQ(response.status, 403);
    }
}
