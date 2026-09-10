#pragma once

#include <cstdint>

namespace vpsm::server::domain {
    struct PeerEndpoint {
        std::uint32_t ip = 0;
        std::uint16_t port = 0;
        std::uint64_t sessionId = 0;
    };
}
