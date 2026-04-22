#include "../../src/application/DataPlane/AuthServiceV2.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {
    using vpsm::server::application::AuthServiceV2;
    using vpsm::server::domain::PacketIn;

    std::vector<std::uint8_t> makePacket(
        std::uint8_t version,
        std::uint64_t sessionId,
        std::uint64_t seq,
        std::uint8_t packetType,
        std::uint32_t vnet,
        std::uint32_t src,
        std::uint32_t dst
    ) {
        std::vector<std::uint8_t> data;
        data.reserve(30);

        data.push_back(version);
        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<std::uint8_t>((sessionId >> (i * 8)) & 0xFF));
        }
        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<std::uint8_t>((seq >> (i * 8)) & 0xFF));
        }

        data.push_back(packetType);
        auto putU32 = [&data](std::uint32_t v) {
            data.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
            data.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
            data.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
            data.push_back(static_cast<std::uint8_t>(v & 0xFF));
        };
        putU32(vnet);
        putU32(src);
        putU32(dst);

        return data;
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_ValidPacket_ResultPresentTrue) {
        std::unordered_map<std::uint64_t, vpsm::server::domain::SessionAuthStateV2> sessions;
        sessions.emplace(123u, vpsm::server::domain::SessionAuthStateV2{.peerId = 42u, .highestSeq = 1u});
        AuthServiceV2 auth(sessions);

        auto raw = makePacket(2, 123u, 2u, 0, 10, 100, 200);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto result = auth.verifyAndDecrypt(pkt);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->outer.sessionId, 123u);
        EXPECT_EQ(result->outer.seq, 2u);
        EXPECT_EQ(result->inner.vNetworkId, 10u);
        ASSERT_TRUE(result->authenticatedPeerId.has_value());
        EXPECT_EQ(*result->authenticatedPeerId, 42u);
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_UnknownSession_NulloptTrue) {
        std::unordered_map<std::uint64_t, vpsm::server::domain::SessionAuthStateV2> sessions;
        AuthServiceV2 auth(sessions);

        auto raw = makePacket(2, 555u, 2u, 0, 10, 100, 200);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto result = auth.verifyAndDecrypt(pkt);
        EXPECT_FALSE(result.has_value());
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_ReplayDetected_NulloptTrue) {
        std::unordered_map<std::uint64_t, vpsm::server::domain::SessionAuthStateV2> sessions;
        sessions.emplace(777u, vpsm::server::domain::SessionAuthStateV2{.peerId = 77u, .highestSeq = 9u});
        AuthServiceV2 auth(sessions);

        auto raw = makePacket(2, 777u, 9u, 0, 10, 100, 200);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto result = auth.verifyAndDecrypt(pkt);
        EXPECT_FALSE(result.has_value());
    }
}
