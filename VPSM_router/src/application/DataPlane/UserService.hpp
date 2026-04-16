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

        port::CreatePeerResult createPeer(
            const std::string& nickname,
            const std::string& passwordHash
        ) override;

        std::optional<std::uint64_t> findPeerIdByNickname(const std::string& nickname) const override;
        bool verifyPeerPassword(std::uint64_t peerId, const std::string& passwordHash) const override;

        port::ActionResult deletePeer(std::uint64_t peerId) override;

        port::CreateNetworkResult createNetwork(
            std::uint64_t ownerPeerId,
            const std::string& name,
            const std::string& passwordHash
        ) override;

        port::ActionResult deleteNetwork(
            std::uint64_t requesterPeerId,
            std::uint64_t networkId
        ) override;

        port::JoinNetworkResult joinNetwork(
            std::uint64_t peerId,
            std::uint64_t networkId,
            const std::string& passwordHash
        ) override;

        port::ActionResult leaveNetwork(
            std::uint64_t peerId,
            std::uint64_t networkId
        ) override;

        std::vector<domain::VNetwork> listUserNetworks(std::uint64_t peerId) const override;
        std::vector<domain::Peer> listNetworkPeers(std::uint64_t networkId) const override;

    private:
        port::IPeerRepository& peerRepository_;
        port::IVNetworkRepository& networkRepository_;
        port::IMembershipStore& membershipStore_;
    };
}