#pragma once

#include "../domain/model/headerV2.hpp"
#include "../domain/model/packetIn.hpp"

#include <optional>

namespace vpsm::server::application {
    class PacketParserV2 {
    public:
        static std::optional<domain::OutPacketHeaderV2> parseOuter(const domain::PacketIn& pkt);

        static std::optional<domain::InnerPacketHeaderV2> parseInner(
            const std::uint8_t* data,
            std::size_t len,
            std::size_t off = 0
        );

    private:
        static std::uint32_t readU32BE(const std::uint8_t* p);
        static std::uint64_t readU64BE(const std::uint8_t* p);
    };
}
