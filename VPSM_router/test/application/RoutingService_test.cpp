#include "../../src/application/DataPlane/RoutingService.hpp"
#include "../../src/application/DataPlane/PacketParserV2.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {
    using vpsm::server::application::RoutingService;
    using vpsm::server::domain::AuthResultV2;
    using vpsm::server::domain::Drop;
    using vpsm::server::domain::Forward;
    using vpsm::server::domain::InnerPacketHeaderV2;
    using vpsm::server::domain::OutPacketHeaderV2;
    using vpsm::server::domain::PacketIn;

    class AuthServiceV2Fake final : public vpsm::server::port::IAuthServiceV2 {
    public:
        std::optional<AuthResultV2> verifyAndDecrypt(const PacketIn& packet) override {
            ++verifyCalls;
            if (passthroughForLegacyUnitTest) {
                const auto outer = vpsm::server::application::PacketParserV2::parseOuter(packet);
                const auto inner = packet.buf
                    ? vpsm::server::application::PacketParserV2::parseInner(
                        packet.buf->data(), packet.size,
                        vpsm::server::domain::OUTER_HEADER_V2_SIZE)
                    : std::nullopt;
                if (!outer || !inner) return std::nullopt;
                return AuthResultV2{
                    .outer = *outer,
                    .inner = *inner,
                    .authenticatedPeerId = std::nullopt,
                    .plaintextInnerAndPayload = std::make_shared<std::vector<std::uint8_t>>(
                        packet.buf->begin() + static_cast<std::ptrdiff_t>(vpsm::server::domain::OUTER_HEADER_V2_SIZE),
                        packet.buf->begin() + static_cast<std::ptrdiff_t>(packet.size)),
                };
            }
            if (verifyResult.has_value() && !verifyResult->plaintextInnerAndPayload) {
                const auto& inner = verifyResult->inner;
                auto plaintext = std::make_shared<std::vector<std::uint8_t>>();
                plaintext->push_back(static_cast<std::uint8_t>(inner.packetType));
                auto putU32 = [&plaintext](std::uint32_t value) {
                    for (int i = 3; i >= 0; --i) {
                        plaintext->push_back(static_cast<std::uint8_t>(value >> (i * 8)));
                    }
                };
                putU32(inner.vNetworkId);
                putU32(inner.srcVip);
                putU32(inner.dstVip);
                if (inner.packetType == vpsm::server::domain::PacketTypeV2::DATA) {
                    std::vector<std::uint8_t> ip(20, 0);
                    ip[0] = 0x45;
                    ip[3] = 20;
                    ip[8] = 64;
                    ip[9] = 6;
                    auto writeAddress = [&ip](std::size_t offset, std::uint32_t address) {
                        for (int i = 0; i < 4; ++i) {
                            ip[offset + i] = static_cast<std::uint8_t>(address >> ((3 - i) * 8));
                        }
                    };
                    writeAddress(12, inner.srcVip);
                    writeAddress(16, inner.dstVip);
                    plaintext->insert(plaintext->end(), ip.begin(), ip.end());
                }
                verifyResult->plaintextInnerAndPayload = std::move(plaintext);
            }
            return verifyResult;
        }

        std::optional<vpsm::server::domain::buffer> encryptForSession(
            std::uint64_t sessionId,
            const std::vector<std::uint8_t>& plaintext
        ) override {
            ++encryptCalls;
            lastEncryptedSessionId = sessionId;
            auto out = std::make_shared<std::vector<std::uint8_t>>(plaintext);
            return out;
        }

        std::optional<AuthResultV2> verifyResult;
        int verifyCalls = 0;
        int encryptCalls = 0;
        std::uint64_t lastEncryptedSessionId = 0;
        bool passthroughForLegacyUnitTest = false;
    };

    class MembershipStoreFake final : public vpsm::server::port::IMembershipStore {
    public:
        std::optional<std::uint32_t> allocateVip(std::uint32_t, std::uint64_t) override { return std::nullopt; }
        bool releaseVip(std::uint32_t, std::uint64_t) override { return false; }
        bool removeNetwork(std::uint32_t) override { return false; }
        bool bindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }
        bool unbindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }
        bool hasPeer(std::uint32_t, std::uint64_t) const override { return false; }
        std::optional<std::uint32_t> resolveVip(std::uint32_t networkId, std::uint64_t peerId) const override {
            const auto key = keyOf(networkId, peerId);
            const auto it = resolveVipByKey.find(key);
            if (it == resolveVipByKey.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        std::optional<std::uint64_t> resolvePeer(std::uint32_t networkId, std::uint32_t vip) const override {
            const auto key = keyOf(networkId, vip);
            const auto it = resolvePeerByKey.find(key);
            if (it == resolvePeerByKey.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        std::vector<vpsm::server::domain::Peer> listPeers(std::uint32_t) const override {
            return {};
        }

        std::unordered_map<std::uint64_t, std::uint64_t> resolvePeerByKey;
        std::unordered_map<std::uint64_t, std::uint32_t> resolveVipByKey;

    private:
        static std::uint64_t keyOf(std::uint32_t networkId, std::uint32_t vip) {
            return (static_cast<std::uint64_t>(networkId) << 32) | vip;
        }
    };

    class PeerEndpointRegistryFake final : public vpsm::server::port::IPeerEndpointRegistry {
    public:
        void upsert(
            std::uint64_t peerId,
            vpsm::server::domain::PeerEndpoint endpoint,
            std::chrono::steady_clock::time_point
        ) override {
            byPeerId[peerId] = endpoint;
        }

        std::optional<vpsm::server::domain::PeerEndpoint> resolve(std::uint64_t peerId) const override {
            const auto it = byPeerId.find(peerId);
            if (it == byPeerId.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        void pruneExpired(std::chrono::steady_clock::time_point, std::chrono::seconds) override {}

        std::unordered_map<std::uint64_t, vpsm::server::domain::PeerEndpoint> byPeerId;
    };

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
        if (type == 0) {
            std::vector<std::uint8_t> ipv4(20, 0);
            ipv4[0] = 0x45;
            ipv4[2] = 0;
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
        return data;
    }

    TEST(RoutingServiceTest, route_parserVernulNullopt_Drop) {

        MembershipStoreFake membership;
        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth);
        PacketIn pkt{.buf = nullptr, .size = 30, .type = vpsm::server::domain::UDP, .sourceIp = 0};

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::PARSE);
    }

    TEST(RoutingServiceTest, route_srcVipNeRezolvitsya_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 1, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::MEMBERSHIP);
    }

    TEST(RoutingServiceTest, route_dstVipNeRezolvitsya_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 1, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::MEMBERSHIP);
    }

    TEST(RoutingServiceTest, route_validniyPaketIObaPeerEst_Forward) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        endpoints.byPeerId.emplace(2u, vpsm::server::domain::PeerEndpoint{
            .ip = 0x0A0000C8u, .port = 4000u, .sessionId = 900u
        });
        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth, &endpoints);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 777, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
            .sourcePort = 0,
        };

        const auto action = service.route(pkt);

        ASSERT_TRUE(std::holds_alternative<Forward>(action));
        const auto& forward = std::get<Forward>(action);
        EXPECT_TRUE(forward.packet.buf);
        EXPECT_EQ(forward.packet.type, pkt.type);
        EXPECT_EQ(forward.packet.destIp, 0x0A0000C8u);
        EXPECT_EQ(forward.packet.destPort, 4000u);
    }

    TEST(RoutingServiceTest, route_validPacketAndAuthFailed_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        AuthServiceV2Fake auth;
        auth.verifyResult = std::nullopt;
        RoutingService service(membership, auth);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 10, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(auth.verifyCalls, 1);
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::AUTH);
    }

    TEST(RoutingServiceTest, route_validPacketAndAuthPassed_Forward) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        membership.resolveVipByKey.emplace((10ull << 32) | 1u, 100u);
        endpoints.byPeerId.emplace(2u, vpsm::server::domain::PeerEndpoint{
            .ip = 0x0A0000C8u, .port = 4000u, .sessionId = 901u
        });
        AuthServiceV2Fake auth;
        auth.verifyResult = AuthResultV2{
            .outer = OutPacketHeaderV2{.packetVersion = 2, .sessionId = 50, .seq = 11},
            .inner = InnerPacketHeaderV2{.packetType = vpsm::server::domain::PacketTypeV2::DATA, .vNetworkId = 10, .srcVip = 100, .dstVip = 200},
            .authenticatedPeerId = 1u,
        };
        RoutingService service(membership, auth, &endpoints);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 11, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        ASSERT_TRUE(std::holds_alternative<Forward>(action));
        EXPECT_EQ(auth.verifyCalls, 1);
        const auto& forward = std::get<Forward>(action);
        EXPECT_EQ(forward.packet.destIp, 0x0A0000C8u);
        EXPECT_EQ(forward.packet.destPort, 4000u);
        EXPECT_EQ(auth.encryptCalls, 1);
        EXPECT_EQ(auth.lastEncryptedSessionId, 901u);
    }

    TEST(RoutingServiceTest, route_authPassedButSrcVipMismatchWithSessionBoundPeer_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        membership.resolveVipByKey.emplace((10ull << 32) | 1u, 101u);
        AuthServiceV2Fake auth;
        auth.verifyResult = AuthResultV2{
            .outer = OutPacketHeaderV2{.packetVersion = 2, .sessionId = 50, .seq = 12},
            .inner = InnerPacketHeaderV2{.packetType = vpsm::server::domain::PacketTypeV2::DATA, .vNetworkId = 10, .srcVip = 100, .dstVip = 200},
            .authenticatedPeerId = 1u,
        };
        RoutingService service(membership, auth);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 12, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(auth.verifyCalls, 1);
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::MEMBERSHIP);
    }

    TEST(RoutingServiceTest, route_validPacketWithAuth_UpdatesSourceEndpointRegistry) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        membership.resolveVipByKey.emplace((10ull << 32) | 1u, 100u);
        endpoints.byPeerId.emplace(2u, vpsm::server::domain::PeerEndpoint{
            .ip = 0x0A0000C8u, .port = 4000u, .sessionId = 902u
        });

        AuthServiceV2Fake auth;
        auth.verifyResult = AuthResultV2{
            .outer = OutPacketHeaderV2{.packetVersion = 2, .sessionId = 70, .seq = 42},
            .inner = InnerPacketHeaderV2{.packetType = vpsm::server::domain::PacketTypeV2::DATA, .vNetworkId = 10, .srcVip = 100, .dstVip = 200},
            .authenticatedPeerId = 1u,
        };

        RoutingService service(membership, auth, &endpoints);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 42, 70);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0xC0A8010Au,
            .sourcePort = 51820u,
        };

        const auto action = service.route(pkt);

        ASSERT_TRUE(std::holds_alternative<Forward>(action));
        const auto srcEndpoint = endpoints.resolve(1u);
        ASSERT_TRUE(srcEndpoint.has_value());
        EXPECT_EQ(srcEndpoint->ip, 0xC0A8010Au);
        EXPECT_EQ(srcEndpoint->port, 51820u);
        EXPECT_EQ(srcEndpoint->sessionId, 70u);
        EXPECT_EQ(auth.lastEncryptedSessionId, 902u);
    }

    TEST(RoutingServiceTest, route_dstPeerWithoutEndpoint_Drop) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        membership.resolveVipByKey.emplace((10ull << 32) | 1u, 100u);

        AuthServiceV2Fake auth;
        auth.verifyResult = AuthResultV2{
            .outer = OutPacketHeaderV2{.packetVersion = 2, .sessionId = 71, .seq = 43},
            .inner = InnerPacketHeaderV2{.packetType = vpsm::server::domain::PacketTypeV2::DATA, .vNetworkId = 10, .srcVip = 100, .dstVip = 200},
            .authenticatedPeerId = 1u,
        };

        RoutingService service(membership, auth, &endpoints);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 43, 71);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0xC0A8010Au,
            .sourcePort = 51820u,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::NO_ENDPOINT);
    }

    TEST(RoutingServiceTest, route_dstUnknownEndpoint_LearnsSourceEndpointEvenWhenDrop) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);

        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth, &endpoints);

        auto raw = makePacketV2(2, 0, 10, 100, 200, 100, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A00000Au,
            .sourcePort = 45000u,
        };

        const auto action = service.route(pkt);

        ASSERT_TRUE(std::holds_alternative<Drop>(action));
        EXPECT_EQ(std::get<Drop>(action).reason, vpsm::server::domain::DropReason::NO_ENDPOINT);

        const auto learnedSrcEndpoint = endpoints.resolve(1u);
        ASSERT_TRUE(learnedSrcEndpoint.has_value());
        EXPECT_EQ(learnedSrcEndpoint->ip, 0x0A00000Au);
        EXPECT_EQ(learnedSrcEndpoint->port, 45000u);
    }

    TEST(RoutingServiceTest, route_bilateralFirstPackets_SecondDirectionCanForwardAfterLearning) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);

        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth, &endpoints);

        auto firstRaw = makePacketV2(2, 0, 10, 100, 200, 101, 50);
        PacketIn first{
            .buf = std::make_shared<std::vector<std::uint8_t>>(firstRaw),
            .size = firstRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A00000Au,
            .sourcePort = 45000u,
        };

        const auto firstAction = service.route(first);
        ASSERT_TRUE(std::holds_alternative<Drop>(firstAction));
        EXPECT_EQ(std::get<Drop>(firstAction).reason, vpsm::server::domain::DropReason::NO_ENDPOINT);

        auto secondRaw = makePacketV2(2, 0, 10, 200, 100, 102, 50);
        PacketIn second{
            .buf = std::make_shared<std::vector<std::uint8_t>>(secondRaw),
            .size = secondRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A000014u,
            .sourcePort = 46000u,
        };

        const auto secondAction = service.route(second);
        ASSERT_TRUE(std::holds_alternative<Forward>(secondAction));
        const auto& forward = std::get<Forward>(secondAction).packet;
        EXPECT_EQ(forward.destIp, 0x0A00000Au);
        EXPECT_EQ(forward.destPort, 45000u);
    }

    TEST(RoutingServiceTest, route_bootstrapThenReverseTraffic_ForwardToLearnedEndpoint) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);

        // peer1 endpoint is known, peer2 endpoint will be learned from incoming packet.
        endpoints.byPeerId.emplace(1u, vpsm::server::domain::PeerEndpoint{.ip = 0x0A000001u, .port = 50001u});

        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth, &endpoints);

        auto bootstrapRaw = makePacketV2(2, 0, 10, 200, 100, 1, 50);
        PacketIn bootstrap{
            .buf = std::make_shared<std::vector<std::uint8_t>>(bootstrapRaw),
            .size = bootstrapRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x7F000001u,
            .sourcePort = 60002u,
        };

        const auto bootstrapAction = service.route(bootstrap);
        ASSERT_TRUE(std::holds_alternative<Forward>(bootstrapAction));

        auto reverseRaw = makePacketV2(2, 0, 10, 100, 200, 2, 50);
        PacketIn reverse{
            .buf = std::make_shared<std::vector<std::uint8_t>>(reverseRaw),
            .size = reverseRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A000001u,
            .sourcePort = 50001u,
        };

        const auto reverseAction = service.route(reverse);
        ASSERT_TRUE(std::holds_alternative<Forward>(reverseAction));
        const auto& forward = std::get<Forward>(reverseAction);
        EXPECT_EQ(forward.packet.destIp, 0x7F000001u);
        EXPECT_EQ(forward.packet.destPort, 60002u);
    }

    TEST(RoutingServiceTest, route_sameIpDifferentPorts_LastSourcePortWinsForDestination) {

        MembershipStoreFake membership;
        PeerEndpointRegistryFake endpoints;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);

        endpoints.byPeerId.emplace(1u, vpsm::server::domain::PeerEndpoint{.ip = 0x0A000001u, .port = 50001u});

        AuthServiceV2Fake auth;
        auth.passthroughForLegacyUnitTest = true;
        RoutingService service(membership, auth, &endpoints);

        auto firstBootstrapRaw = makePacketV2(2, 0, 10, 200, 100, 10, 50);
        PacketIn firstBootstrap{
            .buf = std::make_shared<std::vector<std::uint8_t>>(firstBootstrapRaw),
            .size = firstBootstrapRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x7F000001u,
            .sourcePort = 61000u,
        };
        ASSERT_TRUE(std::holds_alternative<Forward>(service.route(firstBootstrap)));

        auto secondBootstrapRaw = makePacketV2(2, 0, 10, 200, 100, 11, 50);
        PacketIn secondBootstrap{
            .buf = std::make_shared<std::vector<std::uint8_t>>(secondBootstrapRaw),
            .size = secondBootstrapRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x7F000001u,
            .sourcePort = 62000u,
        };
        ASSERT_TRUE(std::holds_alternative<Forward>(service.route(secondBootstrap)));

        auto reverseRaw = makePacketV2(2, 0, 10, 100, 200, 12, 50);
        PacketIn reverse{
            .buf = std::make_shared<std::vector<std::uint8_t>>(reverseRaw),
            .size = reverseRaw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0x0A000001u,
            .sourcePort = 50001u,
        };

        const auto reverseAction = service.route(reverse);
        ASSERT_TRUE(std::holds_alternative<Forward>(reverseAction));
        const auto& forward = std::get<Forward>(reverseAction);
        EXPECT_EQ(forward.packet.destIp, 0x7F000001u);
        EXPECT_EQ(forward.packet.destPort, 62000u);
    }

}
