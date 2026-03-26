#include "../../src/application/DataPlane/RoutingService.hpp"

#include <gtest/gtest.h>

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
        std::optional<AuthResultV2> verifyAndDecrypt(const PacketIn&) override {
            ++verifyCalls;
            return verifyResult;
        }

        std::optional<AuthResultV2> verifyResult;
        int verifyCalls = 0;
    };

    class MembershipStoreFake final : public vpsm::server::port::IMembershipStore {
    public:
        std::optional<std::uint32_t> allocateVip(std::uint32_t, std::uint64_t) override { return std::nullopt; }
        bool releaseVip(std::uint32_t, std::uint64_t) override { return false; }
        bool bindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }
        bool unbindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }
        bool hasPeer(std::uint32_t, std::uint64_t) const override { return false; }
        std::optional<std::uint32_t> resolveVip(std::uint32_t, std::uint64_t) const override { return std::nullopt; }

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

    private:
        static std::uint64_t keyOf(std::uint32_t networkId, std::uint32_t vip) {
            return (static_cast<std::uint64_t>(networkId) << 32) | vip;
        }
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
        return data;
    }

    TEST(RoutingServiceTest, route_parserVernulNullopt_Drop) {

        MembershipStoreFake membership;
        RoutingService service(membership);
        PacketIn pkt{.buf = nullptr, .size = 30, .type = vpsm::server::domain::UDP, .sourceIp = 0};

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
    }

    TEST(RoutingServiceTest, route_srcVipNeRezolvitsya_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        RoutingService service(membership);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 1, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
    }

    TEST(RoutingServiceTest, route_dstVipNeRezolvitsya_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        RoutingService service(membership);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 1, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        EXPECT_TRUE(std::holds_alternative<Drop>(action));
    }

    TEST(RoutingServiceTest, route_validniyPaketIObaPeerEst_Forward) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        RoutingService service(membership);
        auto raw = makePacketV2(2, 0, 10, 100, 200, 777, 50);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto action = service.route(pkt);

        ASSERT_TRUE(std::holds_alternative<Forward>(action));
        const auto& forward = std::get<Forward>(action);
        EXPECT_EQ(forward.packet.buf, pkt.buf);
        EXPECT_EQ(forward.packet.size, pkt.size);
        EXPECT_EQ(forward.packet.type, pkt.type);
        EXPECT_EQ(forward.packet.dest, 200u);
    }

    TEST(RoutingServiceTest, route_validPacketAndAuthFailed_Drop) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        AuthServiceV2Fake auth;
        auth.verifyResult = std::nullopt;
        RoutingService service(membership, &auth);

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
    }

    TEST(RoutingServiceTest, route_validPacketAndAuthPassed_Forward) {

        MembershipStoreFake membership;
        membership.resolvePeerByKey.emplace((10ull << 32) | 100u, 1u);
        membership.resolvePeerByKey.emplace((10ull << 32) | 200u, 2u);
        AuthServiceV2Fake auth;
        auth.verifyResult = AuthResultV2{
            .outer = OutPacketHeaderV2{.packetVersion = 2, .sessionId = 50, .seq = 11},
            .inner = InnerPacketHeaderV2{.packetType = vpsm::server::domain::PacketTypeV2::DATA, .vNetworkId = 10, .srcVip = 100, .dstVip = 200},
        };
        RoutingService service(membership, &auth);

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
    }

}
