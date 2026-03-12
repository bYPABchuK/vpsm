#pragma once

#include "../type/buffer.hpp"
#include "../type/transportType.hpp"
#include <cstddef>
namespace vpsm::server::domain {
    struct PacketOut {
        buffer buf;
        std::size_t size;
        transportType type;
        std::uint32_t dest;
    };
}