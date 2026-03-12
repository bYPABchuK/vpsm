#pragma once

#include <cstdint>

namespace vpsm::server::domain {
    enum class PacketType : std::uint8_t {
        DATA = 0,
        PROBE = 1,
        KEEPALIVE = 2,
    };
    
    static constexpr std::size_t HEADER_SIZE = 26;
    static constexpr std::uint8_t VERSION = 1;

    struct PacketHeader {
        std::uint8_t packetVersion = 1;
        PacketType packetType = PacketType::DATA;
        std::uint32_t vNetworkId = 0;
        std::uint32_t srcVip = 0;
        std::uint32_t dstVip = 0;
        std::uint64_t seq = 0;
        std::uint32_t keyId = 0;
    };
}