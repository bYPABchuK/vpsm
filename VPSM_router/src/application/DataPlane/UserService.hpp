#pragma once

#include "../../port/IUserService.hpp"
#include "../../port/IPeerRepository.hpp"
#include "../../port/IVNetworkRepository.hpp"
#include "../../port/IMembershipStore.hpp"

namespace vpsm::server::application {
    class UserService final : public port::IUserService {
    public:
        UserService(
            port::IPeerRepository& peerRepository,
            port::IVNetworkRepository& networkRepository,
            port::IMembershipStore& membershipStore
        );

        std::optional<std::uint64_t> createPeer(
            const std::string& nickname,
            const std::string& passwordHash
        ) override;

        bool deletePeer(std::uint64_t peerId) override;

        std::optional<std::uint64_t> createNetwork(
            std::uint64_t ownerPeerId,
            const std::string& name,
            const std::string& passwordHash
        ) override;

        bool deleteNetwork(
            std::uint64_t requesterPeerId,
            std::uint64_t networkId
        ) override;

        std::optional<std::uint32_t> joinNetwork(
            std::uint64_t peerId,
            std::uint64_t networkId,
            const std::string& passwordHash
        ) override;

        bool leaveNetwork(
            std::uint64_t peerId,
            std::uint64_t networkId
        ) override;

    private:
        port::IPeerRepository& peerRepository_;
        port::IVNetworkRepository& networkRepository_;
        port::IMembershipStore& membershipStore_;
    };
}