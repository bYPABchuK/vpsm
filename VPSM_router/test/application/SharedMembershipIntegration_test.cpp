#include "../../src/adapter/MembershipStore.hpp"
#include "../../src/adapter/PeerEndpointRegistry.hpp"
#include "../../src/application/DataPlane/RoutingService.hpp"
#include "../../src/application/DataPlane/AuthServiceV2.hpp"
#include "../../src/application/DataPlane/PacketCryptoV2.hpp"
#include "../../src/application/ControlPlane/SessionStore.hpp"
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
        if (type == 0) {
            std::vector<std::uint8_t> ipv4(20, 0);
            ipv4[0] = 0x45;
            ipv4[3] = 20;
            ipv4[8] = 64;
            ipv4[9] = 6;
            auto writeAddress = [&ipv4](std::size_t offset, std::uint32_t address) {
                for (int i = 0; i < 4; ++i) {
                    ipv4[offset + i] = static_cast<std::uint8_t>(address >> ((3 - i) * 8));
                }
            };
            writeAddress(12, srcVip);
            writeAddress(16, dstVip);
            data.insert(data.end(), ipv4.begin(), ipv4.end());
        }
        vpsm::server::application::PacketCryptoV2::Key key{};
        (void)version;
        (void)sessionId;
        (void)seq;
        (void)key;
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
        vpsm::server::application::SessionStore sessions;
        const auto session1 = sessions.createSession(peer1Id);
        const auto session2 = sessions.createSession(peer2Id);
        vpsm::server::application::AuthServiceV2 auth(sessions);
        endpointRegistry.upsert(
            peer2Id,
            vpsm::server::domain::PeerEndpoint{
                .ip = 0x0A0000C8u, .port = 5000u, .sessionId = session2.sessionId
            },
            std::chrono::steady_clock::now()
        );

        vpsm::server::application::RoutingService routingService(
            *sharedMembership,
            auth,
            &endpointRegistry
        );

        const auto plaintext = makePacketV2(2, 0, networkId, vip1, vip2, 1, session1.sessionId);
        const auto raw = vpsm::server::application::PacketCryptoV2::encrypt(
            vpsm::server::domain::OutPacketHeaderV2{2, session1.sessionId, 1},
            plaintext,
            session1.dataPlaneKey,
            vpsm::server::application::PacketCryptoV2::Direction::ClientToRouter
        );
        ASSERT_TRUE(raw.has_value());
        vpsm::server::domain::PacketIn packetIn{
            .buf = std::make_shared<std::vector<std::uint8_t>>(*raw),
            .size = raw->size(),
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

    TEST(SharedMembershipIntegrationTest, sharedPeerUsesOneVipAcrossNetworksWhileOtherPeersStayIsolated) {
        using namespace vpsm::server;
        repository::InMemoryPeerRepository peerRepository;
        repository::InMemoryVNetworkRepository networkRepository;
        adapter::MembershipRegistry membership;
        application::UserService users(peerRepository, networkRepository, membership);

        const auto a = std::get<port::CreatePeerSuccess>(users.createPeer("a", "pw")).peerId;
        const auto b = std::get<port::CreatePeerSuccess>(users.createPeer("b", "pw")).peerId;
        const auto c = std::get<port::CreatePeerSuccess>(users.createPeer("c", "pw")).peerId;
        const auto ab = std::get<port::CreateNetworkSuccess>(users.createNetwork(a, "ab", "netpw")).networkId;
        const auto ac = std::get<port::CreateNetworkSuccess>(users.createNetwork(a, "ac", "netpw")).networkId;
        const auto joinAab = std::get<port::JoinNetworkSuccess>(users.joinNetwork(a, ab, "netpw"));
        const auto joinAac = std::get<port::JoinNetworkSuccess>(users.joinNetwork(a, ac, "netpw"));
        const auto joinB = std::get<port::JoinNetworkSuccess>(users.joinNetwork(b, ab, "netpw"));
        const auto joinC = std::get<port::JoinNetworkSuccess>(users.joinNetwork(c, ac, "netpw"));
        EXPECT_EQ(joinAab.vip, joinAac.vip);
        EXPECT_NE(joinB.vip, joinC.vip);

        application::SessionStore sessions;
        const auto sessionA = sessions.createSession(a);
        const auto sessionB = sessions.createSession(b);
        const auto sessionC = sessions.createSession(c);
        application::AuthServiceV2 auth(sessions);
        adapter::PeerEndpointRegistry endpoints;
        endpoints.upsert(a, domain::PeerEndpoint{.ip = 1, .port = 4001, .sessionId = sessionA.sessionId}, std::chrono::steady_clock::now());
        endpoints.upsert(b, domain::PeerEndpoint{.ip = 2, .port = 4002, .sessionId = sessionB.sessionId}, std::chrono::steady_clock::now());
        endpoints.upsert(c, domain::PeerEndpoint{.ip = 3, .port = 4003, .sessionId = sessionC.sessionId}, std::chrono::steady_clock::now());
        application::RoutingService router(membership, auth, &endpoints);

        const auto route = [&](std::uint32_t networkId, std::uint32_t srcVip, std::uint32_t dstVip,
                               std::uint64_t sequence, const application::SessionStore::SessionData& session) {
            const auto plaintext = makePacketV2(2, 0, networkId, srcVip, dstVip, sequence, session.sessionId);
            const auto raw = application::PacketCryptoV2::encrypt(
                domain::OutPacketHeaderV2{2, session.sessionId, sequence}, plaintext, session.dataPlaneKey,
                application::PacketCryptoV2::Direction::ClientToRouter);
            return router.route(domain::PacketIn{
                .buf = std::make_shared<std::vector<std::uint8_t>>(*raw),
                .size = raw->size(), .type = domain::UDP, .sourceIp = 9, .sourcePort = 5000});
        };

        EXPECT_TRUE(std::holds_alternative<domain::Forward>(route(ab, joinB.vip, joinAab.vip, 1, sessionB)));
        EXPECT_TRUE(std::holds_alternative<domain::Forward>(route(ac, joinC.vip, joinAac.vip, 1, sessionC)));
        const auto bToC = route(ab, joinB.vip, joinC.vip, 2, sessionB);
        ASSERT_TRUE(std::holds_alternative<domain::Drop>(bToC));
        EXPECT_EQ(std::get<domain::Drop>(bToC).reason, domain::DropReason::MEMBERSHIP);
    }
}
