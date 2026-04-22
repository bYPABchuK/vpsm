#pragma once

#include "../port/IPeerEndpointRegistry.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace vpsm::server::adapter {
    class PeerEndpointRegistry final : public port::IPeerEndpointRegistry {
    public:
        void upsert(
            std::uint64_t peerId,
            domain::PeerEndpoint endpoint,
            std::chrono::steady_clock::time_point seenAt
        ) override;

        std::optional<domain::PeerEndpoint> resolve(std::uint64_t peerId) const override;

        void pruneExpired(
            std::chrono::steady_clock::time_point now,
            std::chrono::seconds ttl
        ) override;

    private:
        struct Entry {
            domain::PeerEndpoint endpoint;
            std::chrono::steady_clock::time_point lastSeen;
        };

        mutable std::shared_mutex mutex_;
        std::unordered_map<std::uint64_t, Entry> byPeerId_;
    };
}
