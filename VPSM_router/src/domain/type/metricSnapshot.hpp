#pragma once

#include <cstdint>
namespace vpsm::server::domain {
    struct MetricsSnapshot {
        std::uint64_t packetsReceived = 0;
        std::uint64_t packetsForwarded = 0;
        std::uint64_t packetsDropped = 0;
        std::uint64_t packetsResponded = 0;

        std::uint64_t usersOnline = 0;
    };
}