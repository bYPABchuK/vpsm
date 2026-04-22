#include "../../src/adapter/MembershipStore.hpp"
#include "../../src/adapter/PeerEndpointRegistry.hpp"
#include "../../src/application/DataPlane/RoutingService.hpp"
#include "../../src/application/DataPlane/UserService.hpp"
#include "../../src/repository/InMemoryPeerRepository.hpp"
#include "../../src/repository/InMemoryVNetworkRepository.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace {
    std::vector<std::uint8_t> makePacketV2(
        std::uint8_t version,
        std::uint8_t type,
        std::uint32_t networkId,
        std::uint32_t srcVip,
        std::uint32_t dstVip,
        std::uint64_t seq,
        std::uint64_t sessionId
    ) {
        std::vector<std::uint8_t> data;
        data.reserve(30);
        data.push_back(version);

        auto putU64 = [&data](std::uint64_t v) {
            for (int i = 7; i >= 0; --i) {
                data.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
            }
        };

        putU64(sessionId);
        putU64(seq);
        data.push_back(type);

        auto putU32 = [&data](std::uint32_t v) {
            data.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
            data.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
            data.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
            data.push_back(static_cast<std::uint8_t>(v & 0xFF));
        };

        putU32(networkId);
        putU32(srcVip);
        putU32(dstVip);
        return data;
    }

    TEST(SharedMembershipIntegrationTest, joinNetworkThenRoute_SharedStore_ForwardsPacket) {
        vpsm::server::repository::InMemoryPeerRepository peerRepository;
        vpsm::server::repository::InMemoryVNetworkRepository networkRepository;
        auto sharedMembership = std::make_shared<vpsm::server::adapter::MembershipRegistry>();

        vpsm::server::application::UserService userService(
            peerRepository,
            networkRepository,
            *sharedMembership
        );

        const auto p1 = userService.createPeer("alice", "pw");
        const auto p2 = userService.createPeer("bob", "pw");
        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::CreatePeerSuccess>(p1));
        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::CreatePeerSuccess>(p2));
        const auto peer1Id = std::get<vpsm::server::port::CreatePeerSuccess>(p1).peerId;
        const auto peer2Id = std::get<vpsm::server::port::CreatePeerSuccess>(p2).peerId;

        const auto network = userService.createNetwork(peer1Id, "n1", "netpw");
        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::CreateNetworkSuccess>(network));
        const auto networkId = static_cast<std::uint32_t>(
            std::get<vpsm::server::port::CreateNetworkSuccess>(network).networkId
        );

        const auto join1 = userService.joinNetwork(peer1Id, networkId, "netpw");
        const auto join2 = userService.joinNetwork(peer2Id, networkId, "netpw");
        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::JoinNetworkSuccess>(join1));
        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::JoinNetworkSuccess>(join2));
        const auto vip1 = std::get<vpsm::server::port::JoinNetworkSuccess>(join1).vip;
        const auto vip2 = std::get<vpsm::server::port::JoinNetworkSuccess>(join2).vip;

        vpsm::server::adapter::PeerEndpointRegistry endpointRegistry;
        endpointRegistry.upsert(
            peer2Id,
            vpsm::server::domain::PeerEndpoint{.ip = 0x0A0000C8u, .port = 5000u},
            std::chrono::steady_clock::now()
        );

        vpsm::server::application::RoutingService routingService(
            *sharedMembership,
            nullptr,
            &endpointRegistry
        );

        const auto raw = makePacketV2(2, 0, networkId, vip1, vip2, 1, 10);
        vpsm::server::domain::PacketIn packetIn{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A000001u,
            .sourcePort = 4001u,
        };

        const auto action = routingService.route(packetIn);

        ASSERT_TRUE(std::holds_alternative<vpsm::server::domain::Forward>(action));
        const auto& out = std::get<vpsm::server::domain::Forward>(action).packet;
        EXPECT_EQ(out.destIp, 0x0A0000C8u);
        EXPECT_EQ(out.destPort, 5000u);
    }
}
