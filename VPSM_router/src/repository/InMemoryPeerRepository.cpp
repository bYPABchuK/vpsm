#include "InMemoryPeerRepository.hpp"

#include <mutex>
#include <shared_mutex>

namespace vpsm::server::repository {
    std::optional<std::uint64_t> InMemoryPeerRepository::createPeer(const std::string& nickname, const std::string& passwordHash) {
        std::unique_lock lock(mutex_);

        const auto id = nextId_++;
        passwordByPeerId_[id] = passwordHash;
        nicknameByPeerId_[id] = nickname;
        return id;
    }

    bool InMemoryPeerRepository::deletePeer(std::uint64_t peerId) {
        std::unique_lock lock(mutex_);
        nicknameByPeerId_.erase(peerId);
        return passwordByPeerId_.erase(peerId) > 0;
    }

    std::optional<std::string> InMemoryPeerRepository::getPasswordHash(std::uint64_t peerId) const {
        std::shared_lock lock(mutex_);

        const auto it = passwordByPeerId_.find(peerId);
        if (it == passwordByPeerId_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    bool InMemoryPeerRepository::exists(std::uint64_t peerId) const {
        std::shared_lock lock(mutex_);
        return passwordByPeerId_.find(peerId) != passwordByPeerId_.end();
    }
}
