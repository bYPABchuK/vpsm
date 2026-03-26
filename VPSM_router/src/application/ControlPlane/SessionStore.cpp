#include "SessionStore.hpp"

#include <mutex>

namespace vpsm::server::application {
    SessionStore::SessionData SessionStore::createSession(std::uint64_t peerId) {
        std::unique_lock lock(mutex_);

        std::uint64_t sessionId = 0;
        do {
            sessionId = rng_();
        } while (sessionId == 0 || bySessionId_.find(sessionId) != bySessionId_.end());

        std::uint64_t sessionKey = 0;
        do {
            sessionKey = rng_();
        } while (sessionKey == 0);

        bySessionId_[sessionId] = std::make_pair(peerId, sessionKey);
        return SessionData{.sessionId = sessionId, .sessionKey = sessionKey};
    }

    std::optional<std::uint64_t> SessionStore::authenticate(std::uint64_t sessionId, std::uint64_t sessionKey) const {
        std::shared_lock lock(mutex_);

        const auto it = bySessionId_.find(sessionId);
        if (it == bySessionId_.end()) {
            return std::nullopt;
        }

        if (it->second.second != sessionKey) {
            return std::nullopt;
        }

        return it->second.first;
    }
}
