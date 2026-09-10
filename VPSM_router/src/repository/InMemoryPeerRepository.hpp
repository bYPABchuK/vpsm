#pragma once

#include "../port/IPeerRepository.hpp"

#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace vpsm::server::repository {
    class InMemoryPeerRepository final : public port::IPeerRepository {
    public:
        std::optional<std::uint64_t> createPeer(const std::string& nickname, const std::string& passwordHash) override;
        bool deletePeer(std::uint64_t peerId) override;
        std::optional<std::string> getPasswordHash(std::uint64_t peerId) const override;
        std::optional<std::string> getNickname(std::uint64_t peerId) const override;
        std::optional<std::uint64_t> findPeerIdByNickname(const std::string& nickname) const override;
        bool exists(std::uint64_t peerId) const override;

    private:
        mutable std::shared_mutex mutex_;
        std::uint64_t nextId_ = 1;
        std::unordered_map<std::uint64_t, std::string> passwordByPeerId_;
        std::unordered_map<std::uint64_t, std::string> nicknameByPeerId_;
    };
}
