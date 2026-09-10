#include "../../src/adapter/MembershipStore.hpp"
#include "../../src/adapter/PeerEndpointRegistry.hpp"
#include "../../src/application/ControlPlane/SessionStore.hpp"
#include "../../src/application/DataPlane/AuthServiceV2.hpp"
#include "../../src/application/DataPlane/PacketCryptoV2.hpp"
#include "../../src/application/DataPlane/RoutingService.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {
    using namespace vpsm::server;

    void putU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
        for (int i = 3; i >= 0; --i) out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
    }

    std::vector<std::uint8_t> plaintext(
        std::uint32_t networkId,
        std::uint32_t srcVip,
        std::uint32_t dstVip,
        const std::vector<std::uint8_t>& payload,
        std::uint8_t type = 0
    ) {
        std::vector<std::uint8_t> out{type};
        putU32(out, networkId);
        putU32(out, srcVip);
        putU32(out, dstVip);
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    std::vector<std::uint8_t> ipv4Packet(
        std::uint32_t source,
        std::uint32_t destination,
        const std::vector<std::uint8_t>& payload = {}
    ) {
        std::vector<std::uint8_t> out(20 + payload.size(), 0);
        out[0] = 0x45;
        const auto size = static_cast<std::uint16_t>(out.size());
        out[2] = static_cast<std::uint8_t>(size >> 8);
        out[3] = static_cast<std::uint8_t>(size);
        out[8] = 64;
        out[9] = 6;
        auto writeAddress = [&out](std::size_t offset, std::uint32_t address) {
            for (int i = 0; i < 4; ++i) {
                out[offset + i] = static_cast<std::uint8_t>(address >> ((3 - i) * 8));
            }
        };
        writeAddress(12, source);
        writeAddress(16, destination);
        std::copy(payload.begin(), payload.end(), out.begin() + 20);
        return out;
    }

    domain::PacketIn encryptedPacket(
        const application::SessionStore::SessionData& session,
        std::uint64_t sequence,
        const std::vector<std::uint8_t>& cleartext,
        std::uint32_t sourceIp,
        std::uint16_t sourcePort
    ) {
        const domain::OutPacketHeaderV2 outer{2, session.sessionId, sequence};
        auto encrypted = application::PacketCryptoV2::encrypt(
            outer, cleartext, session.dataPlaneKey,
            application::PacketCryptoV2::Direction::ClientToRouter
        );
        EXPECT_TRUE(encrypted.has_value());
        auto buffer = std::make_shared<std::vector<std::uint8_t>>(std::move(*encrypted));
        return domain::PacketIn{
            .buf = buffer,
            .size = buffer->size(),
            .type = domain::UDP,
            .sourceIp = sourceIp,
            .sourcePort = sourcePort,
        };
    }

    TEST(SecureRoutingIntegrationTest, ValidPacketIsReencryptedForDestinationSession) {
        constexpr std::uint32_t vipA = 0x0AF00A64u; // 10.240.10.100
        constexpr std::uint32_t vipB = 0x0AF00AC8u; // 10.240.10.200
        application::SessionStore sessions;
        const auto sessionA = sessions.createSession(1);
        const auto sessionB = sessions.createSession(2);
        application::AuthServiceV2 auth(sessions);
        adapter::MembershipRegistry membership;
        ASSERT_TRUE(membership.bindPeer(10, 1, vipA));
        ASSERT_TRUE(membership.bindPeer(10, 2, vipB));
        adapter::PeerEndpointRegistry endpoints;
        application::RoutingService routing(membership, auth, &endpoints);

        // B authenticates first. The payload cannot be delivered yet, but its
        // endpoint and active destination session are learned safely.
        const auto bootstrap = routing.route(encryptedPacket(
            sessionB, 1, plaintext(10, vipB, vipA, ipv4Packet(vipB, vipA)), 0x0A000002u, 42002
        ));
        ASSERT_TRUE(std::holds_alternative<domain::Drop>(bootstrap));
        EXPECT_EQ(std::get<domain::Drop>(bootstrap).reason, domain::DropReason::NO_ENDPOINT);

        const auto action = routing.route(encryptedPacket(
            sessionA, 1, plaintext(10, vipA, vipB, ipv4Packet(vipA, vipB, {'s', 's', 'h'})), 0x0A000001u, 42001
        ));
        ASSERT_TRUE(std::holds_alternative<domain::Forward>(action));
        const auto& forwarded = std::get<domain::Forward>(action).packet;
        EXPECT_EQ(forwarded.destIp, 0x0A000002u);
        EXPECT_EQ(forwarded.destPort, 42002u);

        const auto decryptedByB = application::PacketCryptoV2::decrypt(
            forwarded.buf->data(), forwarded.size, sessionB.dataPlaneKey,
            application::PacketCryptoV2::Direction::RouterToClient
        );
        ASSERT_TRUE(decryptedByB.has_value());
        EXPECT_EQ(*decryptedByB, plaintext(10, vipA, vipB, ipv4Packet(vipA, vipB, {'s', 's', 'h'})));
        EXPECT_FALSE(application::PacketCryptoV2::decrypt(
            forwarded.buf->data(), forwarded.size, sessionA.dataPlaneKey,
            application::PacketCryptoV2::Direction::RouterToClient
        ).has_value());
    }

    TEST(SecureRoutingIntegrationTest, ForgedPacketCannotHijackAuthenticatedEndpoint) {
        constexpr std::uint32_t vipA = 0x0AF00A64u; // 10.240.10.100
        constexpr std::uint32_t vipB = 0x0AF00AC8u; // 10.240.10.200
        application::SessionStore sessions;
        const auto sessionA = sessions.createSession(1);
        const auto sessionB = sessions.createSession(2);
        application::AuthServiceV2 auth(sessions);
        adapter::MembershipRegistry membership;
        ASSERT_TRUE(membership.bindPeer(10, 1, vipA));
        ASSERT_TRUE(membership.bindPeer(10, 2, vipB));
        adapter::PeerEndpointRegistry endpoints;
        application::RoutingService routing(membership, auth, &endpoints);

        routing.route(encryptedPacket(
            sessionB, 1, plaintext(10, vipB, vipA, ipv4Packet(vipB, vipA)), 0x0A000002u, 42002
        ));
        const auto original = endpoints.resolve(2);
        ASSERT_TRUE(original.has_value());

        auto forged = encryptedPacket(
            sessionB, 2, plaintext(10, vipB, vipA, ipv4Packet(vipB, vipA, {'x'})), 0x0A000099u, 49999
        );
        forged.buf->back() ^= 1;
        const auto action = routing.route(forged);
        ASSERT_TRUE(std::holds_alternative<domain::Drop>(action));
        EXPECT_EQ(std::get<domain::Drop>(action).reason, domain::DropReason::AUTH);

        const auto after = endpoints.resolve(2);
        ASSERT_TRUE(after.has_value());
        EXPECT_EQ(after->ip, original->ip);
        EXPECT_EQ(after->port, original->port);
        EXPECT_EQ(after->sessionId, original->sessionId);
    }

    TEST(SecureRoutingIntegrationTest, InnerIpv4SpoofIsRejectedBeforeEndpointLearning) {
        constexpr std::uint32_t vipA = 0x0AF00A64u;
        constexpr std::uint32_t vipB = 0x0AF00AC8u;
        application::SessionStore sessions;
        const auto sessionA = sessions.createSession(1);
        application::AuthServiceV2 auth(sessions);
        adapter::MembershipRegistry membership;
        ASSERT_TRUE(membership.bindPeer(10, 1, vipA));
        ASSERT_TRUE(membership.bindPeer(10, 2, vipB));
        adapter::PeerEndpointRegistry endpoints;
        application::RoutingService routing(membership, auth, &endpoints);

        const auto action = routing.route(encryptedPacket(
            sessionA, 1,
            plaintext(10, vipA, vipB, ipv4Packet(vipB, vipA)),
            0x0A000099u, 49999
        ));

        ASSERT_TRUE(std::holds_alternative<domain::Drop>(action));
        EXPECT_EQ(std::get<domain::Drop>(action).reason, domain::DropReason::MEMBERSHIP);
        EXPECT_FALSE(endpoints.resolve(1).has_value());
    }

    TEST(SecureRoutingIntegrationTest, EmptyAuthenticatedSelfKeepaliveLearnsEndpoint) {
        constexpr std::uint32_t vip = 0x0AF00A01u;
        application::SessionStore sessions;
        const auto session = sessions.createSession(1);
        application::AuthServiceV2 auth(sessions);
        adapter::MembershipRegistry membership;
        ASSERT_TRUE(membership.bindPeer(10, 1, vip));
        adapter::PeerEndpointRegistry endpoints;
        application::RoutingService routing(membership, auth, &endpoints);

        const auto action = routing.route(encryptedPacket(
            session, 1, plaintext(10, vip, vip, {}, 2), 0x7F000001u, 42001
        ));

        ASSERT_TRUE(std::holds_alternative<domain::Forward>(action));
        const auto learned = endpoints.resolve(1);
        ASSERT_TRUE(learned.has_value());
        EXPECT_EQ(learned->ip, 0x7F000001u);
        EXPECT_EQ(learned->port, 42001u);
        EXPECT_EQ(learned->sessionId, session.sessionId);
    }
}