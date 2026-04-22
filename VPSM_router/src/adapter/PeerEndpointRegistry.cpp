#include "PeerEndpointRegistry.hpp"

#include <mutex>

namespace vpsm::server::adapter {
    void PeerEndpointRegistry::upsert(
        std::uint64_t peerId,
        domain::PeerEndpoint endpoint,
        std::chrono::steady_clock::time_point seenAt
    ) {
        std::unique_lock lock(mutex_);
        byPeerId_[peerId] = Entry{
            .endpoint = endpoint,
            .lastSeen = seenAt,
        };
    }

    std::optional<domain::PeerEndpoint> PeerEndpointRegistry::resolve(std::uint64_t peerId) const {
        std::shared_lock lock(mutex_);
        const auto it = byPeerId_.find(peerId);
        if (it == byPeerId_.end()) {
            return std::nullopt;
        }
        return it->second.endpoint;
    }

    void PeerEndpointRegistry::pruneExpired(
        std::chrono::steady_clock::time_point now,
        std::chrono::seconds ttl
    ) {
        std::unique_lock lock(mutex_);
        for (auto it = byPeerId_.begin(); it != byPeerId_.end(); ) {
            if (now - it->second.lastSeen > ttl) {
                it = byPeerId_.erase(it);
            } else {
                ++it;
            }
        }
    }
}
