#pragma once

#include "../type/buffer.hpp"
#include "../type/transportType.hpp"
#include <cstddef>

namespace vpsm::server::domain {
    struct PacketOut {
        buffer buf;
        std::size_t size = 0;
        transportType type = UDP;
        std::uint32_t destIp = 0;
        std::uint16_t destPort = 0;
    };
}