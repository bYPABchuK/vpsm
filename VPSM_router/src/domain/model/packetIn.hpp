#pragma once

#include "../type/buffer.hpp"
#include "../type/transportType.hpp"
namespace vpsm::server::domain {
    struct PacketIn {
        buffer buf;
        std::size_t size = 0;
        transportType type = UDP;
        std::uint32_t sourceIp = 0;
        std::uint16_t sourcePort = 0;
    };
}