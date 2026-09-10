#pragma once

#include "../../domain/model/headerV2.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace vpsm::server::application {
    class PacketCryptoV2 {
    public:
        enum class Direction : std::uint32_t {
            ClientToRouter = 1,
            RouterToClient = 2,
        };

        static constexpr std::size_t KEY_SIZE = 32;
        using Key = std::array<std::uint8_t, KEY_SIZE>;

        static std::optional<std::vector<std::uint8_t>> encrypt(
            const domain::OutPacketHeaderV2& outer,
            const std::vector<std::uint8_t>& plaintext,
            const Key& key,
            Direction direction
        );

        static std::optional<std::vector<std::uint8_t>> decrypt(
            const std::uint8_t* packet,
            std::size_t packetSize,
            const Key& key,
            Direction direction
        );

        static std::array<std::uint8_t, domain::OUTER_HEADER_V2_SIZE> encodeOuter(
            const domain::OutPacketHeaderV2& outer
        );
    };
}