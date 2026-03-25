#pragma once

#include <cstddef>
#include <cstdint>

namespace vpsm::server::domain {
    enum class PacketTypeV2 : std::uint8_t {
        DATA = 0,
        PROBE = 1,
        KEEPALIVE = 2,
    };

    static constexpr std::size_t OUTER_HEADER_V2_SIZE = 17;
    static constexpr std::size_t INNER_HEADER_V2_SIZE = 13;
    static constexpr std::uint8_t VERSION_V2 = 2;

    struct OutPacketHeaderV2 {
        std::uint8_t packetVersion = VERSION_V2;
        std::uint64_t sessionId;
        std::uint64_t seq;
    };

    struct InnerPacketHeaderV2 {
        PacketTypeV2 packetType = PacketTypeV2::DATA;
        std::uint32_t vNetworkId;
        std::uint32_t srcVip;
        std::uint32_t dstVip;
    };
}