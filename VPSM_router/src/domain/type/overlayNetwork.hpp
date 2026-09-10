#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace vpsm::server::domain {
    struct OverlayNetworkConfig {
        std::uint32_t networkAddress = 0;
        std::uint8_t prefixLength = 16;
        std::uint16_t mtu = 1400;
        std::uint32_t firstHost = 0;
        std::uint32_t lastHost = 0;
    };

    inline std::optional<OverlayNetworkConfig> overlayConfigForNetworkId(std::uint64_t networkId) {
        if (networkId == 0 || networkId > 255) return std::nullopt;
        // Every logical network shares the server-wide 10.240.0.0/16 overlay.
        // networkId remains the isolated routing/security scope.
        constexpr auto address = 0x0AF00000u;
        return OverlayNetworkConfig{
            .networkAddress = address,
            .prefixLength = 16,
            .mtu = 1400,
            .firstHost = address + 1,
            .lastHost = address + 0xFFFEu,
        };
    }

    inline std::string ipv4ToString(std::uint32_t address) {
        return std::to_string((address >> 24) & 0xffu) + "." +
               std::to_string((address >> 16) & 0xffu) + "." +
               std::to_string((address >> 8) & 0xffu) + "." +
               std::to_string(address & 0xffu);
    }
}