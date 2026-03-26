#pragma once

#include "../../domain/model/packetIn.hpp"
#include "../../domain/model/headerV1.hpp"
#include <optional>
namespace vpsm::server::application {
    class PacketParser {
    public:
        ~PacketParser() = default;
        static std::optional<domain::PacketHeader> parse(const domain::PacketIn& pkt);
    
    private:
    static std::optional<domain::PacketHeader> readHeader(
        const std::uint8_t* data,
        std::size_t len,
        std::size_t off
    );

    static std::uint32_t readU32BE(const std::uint8_t* p);
    static std::uint64_t readU64BE(const std::uint8_t* p);
    };
}