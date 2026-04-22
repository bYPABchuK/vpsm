#pragma once

#include "../domain/type/peerEndpoint.hpp"

#include <chrono>
#include <cstdint>
#include <optional>

namespace vpsm::server::port {
    class IPeerEndpointRegistry {
    public:
        virtual ~IPeerEndpointRegistry() = default;

        virtual void upsert(
            std::uint64_t peerId,
            domain::PeerEndpoint endpoint,
            std::chrono::steady_clock::time_point seenAt
        ) = 0;

        virtual std::optional<domain::PeerEndpoint> resolve(std::uint64_t peerId) const = 0;

        virtual void pruneExpired(
            std::chrono::steady_clock::time_point now,
            std::chrono::seconds ttl
        ) = 0;
    };
}
