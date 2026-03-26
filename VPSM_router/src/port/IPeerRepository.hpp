#pragma once

#include <cstdint>
#include <optional>
#include <string>
namespace vpsm::server::port {
    class IPeerRepository {
    public:
        virtual ~IPeerRepository() = default;

        virtual std::optional<std::uint64_t> createPeer(
            const std::string& nickname,
            const std::string& passwordHash
        ) = 0;

        virtual bool deletePeer(std::uint64_t peerId) = 0;

        virtual std::optional<std::string> getPasswordHash(std::uint64_t peerId) const = 0;
        virtual std::optional<std::uint64_t> findPeerIdByNickname(const std::string& nickname) const = 0;
        virtual bool exists(std::uint64_t peerId) const = 0;
    };
}