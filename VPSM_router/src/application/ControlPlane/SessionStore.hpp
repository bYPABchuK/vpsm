#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <random>

namespace vpsm::server::application {
    class SessionStore {
    public:
        struct SessionData {
            std::uint64_t sessionId;
            std::uint64_t sessionKey;
        };

        SessionData createSession(std::uint64_t peerId);
        std::optional<std::uint64_t> authenticate(std::uint64_t sessionId, std::uint64_t sessionKey) const;

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>> bySessionId_;
        mutable std::mt19937_64 rng_{std::random_device{}()};
    };
}
