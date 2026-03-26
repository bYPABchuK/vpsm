#include "../../src/application/DataPlane/PacketParser.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using vpsm::server::application::PacketParser;
using vpsm::server::domain::PacketIn;
using vpsm::server::domain::PacketType;

std::vector<std::uint8_t> makeHeader(
    std::uint8_t version,
    std::uint8_t type,
    std::uint32_t networkId,
    std::uint32_t srcVip,
    std::uint32_t dstVip,
    std::uint64_t seq,
    std::uint32_t keyId
) {
    std::vector<std::uint8_t> data;
    data.reserve(26);
    data.push_back(version);
    data.push_back(type);

    auto putU32 = [&data](std::uint32_t v) {
        data.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        data.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        data.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        data.push_back(static_cast<std::uint8_t>(v & 0xFF));
    };

    auto putU64 = [&data](std::uint64_t v) {
        for (int i = 7; i >= 0; --i) {
            data.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
        }
    };

    putU32(networkId);
    putU32(srcVip);
    putU32(dstVip);
    putU64(seq);
    putU32(keyId);
    return data;
}

TEST(PacketParserTest, parse_korrektniyHeader_Header) {

    auto raw = makeHeader(1, 0, 10, 100, 200, 555, 42);
    PacketIn pkt{
        .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
        .size = raw.size(),
        .type = vpsm::server::domain::UDP,
        .sourceIp = 0,
    };

    const auto header = PacketParser::parse(pkt);

    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->packetVersion, 1);
    EXPECT_EQ(header->packetType, PacketType::DATA);
    EXPECT_EQ(header->vNetworkId, 10u);
    EXPECT_EQ(header->srcVip, 100u);
    EXPECT_EQ(header->dstVip, 200u);
    EXPECT_EQ(header->seq, 555u);
    EXPECT_EQ(header->keyId, 42u);
}

TEST(PacketParserTest, parse_bufNull_Nullopt) {

    PacketIn pkt{.buf = nullptr, .size = 26, .type = vpsm::server::domain::UDP, .sourceIp = 0};

    const auto header = PacketParser::parse(pkt);

    EXPECT_FALSE(header.has_value());
}

TEST(PacketParserTest, parse_sizeMensheHeader_Nullopt) {

    auto raw = makeHeader(1, 0, 1, 1, 1, 1, 1);
    PacketIn pkt{
        .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
        .size = 10,
        .type = vpsm::server::domain::UDP,
        .sourceIp = 0,
    };

    const auto header = PacketParser::parse(pkt);

    EXPECT_FALSE(header.has_value());
}

TEST(PacketParserTest, parse_nepodderzhivaemayaVersiya_Nullopt) {

    auto raw = makeHeader(2, 0, 1, 1, 1, 1, 1);
    PacketIn pkt{
        .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
        .size = raw.size(),
        .type = vpsm::server::domain::UDP,
        .sourceIp = 0,
    };

    const auto header = PacketParser::parse(pkt);

    EXPECT_FALSE(header.has_value());
}

TEST(PacketParserTest, parse_nepodderzhivaemiyTipPaketa_Nullopt) {

    auto raw = makeHeader(1, 9, 1, 1, 1, 1, 1);
    PacketIn pkt{
        .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
        .size = raw.size(),
        .type = vpsm::server::domain::UDP,
        .sourceIp = 0,
    };

    const auto header = PacketParser::parse(pkt);

    EXPECT_FALSE(header.has_value());
}

TEST(PacketParserTest, parse_bufMensheZayavlennogoSize_Nullopt) {

    auto raw = makeHeader(1, 0, 1, 1, 1, 1, 1);
    PacketIn pkt{
        .buf = std::make_shared<std::vector<std::uint8_t>>(raw),
        .size = raw.size() + 1,
        .type = vpsm::server::domain::UDP,
        .sourceIp = 0,
    };

    const auto header = PacketParser::parse(pkt);

    EXPECT_FALSE(header.has_value());
}

}
