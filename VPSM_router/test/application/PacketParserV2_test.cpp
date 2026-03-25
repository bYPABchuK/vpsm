#include "../../src/application/PacketParserV2.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace {
    using vpsm::server::application::PacketParserV2;
    using vpsm::server::domain::PacketIn;
    using vpsm::server::domain::PacketTypeV2;

    std::vector<std::uint8_t> makeOuter(std::uint8_t version, std::uint64_t sessionId, std::uint64_t seq) {
        std::vector<std::uint8_t> data;
        data.reserve(17);
        data.push_back(version);

        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<std::uint8_t>((sessionId >> (i * 8)) & 0xFF));
        }
        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<std::uint8_t>((seq >> (i * 8)) & 0xFF));
        }
        return data;
    }

    std::vector<std::uint8_t> makeInner(std::uint8_t type, std::uint32_t vnet, std::uint32_t src, std::uint32_t dst) {
        std::vector<std::uint8_t> data;
        data.reserve(13);
        data.push_back(type);

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

    TEST(PacketParserV2Test, parseOuter_ValidPacket_HeaderPresentTrue) {
        auto raw = makeOuter(2, 0x11u, 0x22u);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto header = PacketParserV2::parseOuter(pkt);

        ASSERT_TRUE(header.has_value());
        EXPECT_EQ(header->packetVersion, 2u);
        EXPECT_EQ(header->sessionId, 0x11u);
        EXPECT_EQ(header->seq, 0x22u);
    }

    TEST(PacketParserV2Test, parseInner_ValidPayload_HeaderPresentTrue) {
        auto raw = makeInner(0, 10, 100, 200);

        const auto header = PacketParserV2::parseInner(raw.data(), raw.size());

        ASSERT_TRUE(header.has_value());
        EXPECT_EQ(header->packetType, PacketTypeV2::DATA);
        EXPECT_EQ(header->vNetworkId, 10u);
        EXPECT_EQ(header->srcVip, 100u);
        EXPECT_EQ(header->dstVip, 200u);
    }

    TEST(PacketParserV2Test, parseOuter_InvalidVersion_NulloptTrue) {
        auto raw = makeOuter(1, 1, 1);
        PacketIn pkt{
            .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
            .size = raw.size(),
            .type = vpsm::server::domain::UDP,
            .sourceIp = 0,
        };

        const auto header = PacketParserV2::parseOuter(pkt);
        EXPECT_FALSE(header.has_value());
    }
}
