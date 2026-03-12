#pragma once

#include "../type/buffer.hpp"
#include "../type/transportType.hpp"
namespace vpsm::server::domain {
    struct PacketIn {
        buffer buf;
        std::size_t size;
        transportType type;
        uint32_t sourceIp;
    };
}