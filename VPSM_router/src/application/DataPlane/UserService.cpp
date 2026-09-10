#include "UserService.hpp"

namespace vpsm::server::application {
    UserService::UserService(
        port::IPeerRepository& peerRepository,
        port::IVNetworkRepository& networkRepository,
        port::IMembershipStore& membershipStore
    )
        : peerRepository_(peerRepository),
          networkRepository_(networkRepository),
          membershipStore_(membershipStore) {}

    port::CreatePeerResult UserService::createPeer(
        const std::string& nickname,
        const std::string& passwordHash
    ) {
        const auto peerId = peerRepository_.createPeer(nickname, passwordHash);
        if (!peerId.has_value()) {
            return port::UserServiceError::CreateFailed;
        }

        return port::CreatePeerSuccess{.peerId = *peerId};
    }

    std::optional<std::uint64_t> UserService::findPeerIdByNickname(const std::string& nickname) const {
        return peerRepository_.findPeerIdByNickname(nickname);
    }

    bool UserService::verifyPeerPassword(std::uint64_t peerId, const std::string& passwordHash) const {
        const auto stored = peerRepository_.getPasswordHash(peerId);
        return stored.has_value() && *stored == passwordHash;
    }

    port::ActionResult UserService::deletePeer(std::uint64_t peerId) {
        if (!peerRepository_.exists(peerId)) {
            return port::UserServiceError::PeerNotFound;
        }

        if (!peerRepository_.deletePeer(peerId)) {
            return port::UserServiceError::DeleteFailed;
        }

        return port::ActionSuccess{};
    }

    port::CreateNetworkResult UserService::createNetwork(
        std::uint64_t ownerPeerId,
        const std::string& name,
        const std::string& passwordHash
    ) {
        if (!peerRepository_.exists(ownerPeerId)) {
            return port::UserServiceError::PeerNotFound;
        }

        if (networkRepository_.existsByName(name)) {
            return port::UserServiceError::NetworkNameAlreadyExists;
        }

        const auto networkId = networkRepository_.createNetwork(ownerPeerId, name, passwordHash);
        if (!networkId.has_value()) {
            return port::UserServiceError::CreateFailed;
        }

        // The owner is a member from the moment the network is created.
        if (!membershipStore_.allocateVip(static_cast<std::uint32_t>(*networkId), ownerPeerId).has_value()) {
            networkRepository_.deleteNetwork(*networkId);
            return port::UserServiceError::CreateFailed;
        }

        return port::CreateNetworkSuccess{.networkId = *networkId};
    }

    port::ActionResult UserService::deleteNetwork(
        std::uint64_t requesterPeerId,
        std::uint64_t networkId
    ) {
        auto network = networkRepository_.getNetwork(networkId);
        if (!network.has_value()) {
            return port::UserServiceError::NetworkNotFound;
        }

        if (network->owner.peerId != requesterPeerId) {
            return port::UserServiceError::Forbidden;
        }

        if (!networkRepository_.deleteNetwork(networkId)) {
            return port::UserServiceError::DeleteFailed;
        }

        membershipStore_.removeNetwork(static_cast<std::uint32_t>(networkId));

        return port::ActionSuccess{};
    }

    port::JoinNetworkResult UserService::joinNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId,
        const std::string& passwordHash
    ) {
        if (!peerRepository_.exists(peerId)) {
            return port::UserServiceError::PeerNotFound;
        }

        auto network = networkRepository_.getNetwork(networkId);
        if (!network.has_value()) {
            return port::UserServiceError::NetworkNotFound;
        }

        if (network->password_hash != passwordHash) {
            return port::UserServiceError::InvalidPassword;
        }

        if (membershipStore_.hasPeer(networkId, peerId)) {
            const auto vip = membershipStore_.resolveVip(networkId, peerId);
            if (!vip.has_value()) {
                return port::UserServiceError::InternalError;
            }
            return port::JoinNetworkSuccess{
                .vip = *vip,
                .networkAddress = network->networkAddress,
                .prefixLength = network->prefixLength,
                .mtu = network->mtu,
                .alreadyExists = true,
                .networkId = networkId,
            };
        }

        const auto vip = membershipStore_.allocateVip(networkId, peerId);
        if (!vip.has_value()) {
            return port::UserServiceError::CreateFailed;
        }

        return port::JoinNetworkSuccess{
            .vip = *vip,
            .networkAddress = network->networkAddress,
            .prefixLength = network->prefixLength,
            .mtu = network->mtu,
            .alreadyExists = false,
            .networkId = networkId,
        };
    }

    port::JoinNetworkResult UserService::joinNetworkByName(
        std::uint64_t peerId,
        const std::string& networkName,
        const std::string& passwordHash
    ) {
        const auto network = networkRepository_.getNetworkByName(networkName);
        if (!network.has_value()) return port::UserServiceError::NetworkNotFound;
        return joinNetwork(peerId, network->id, passwordHash);
    }

    port::ActionResult UserService::leaveNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId
    ) {
        if (!peerRepository_.exists(peerId)) {
            return port::UserServiceError::PeerNotFound;
        }

        if (!networkRepository_.exists(networkId)) {
            return port::UserServiceError::NetworkNotFound;
        }

        if (!membershipStore_.hasPeer(networkId, peerId)) {
            return port::UserServiceError::NotMember;
        }

        if (!membershipStore_.releaseVip(networkId, peerId)) {
            return port::UserServiceError::DeleteFailed;
        }

        return port::ActionSuccess{};
    }

    std::vector<domain::NetworkMembership> UserService::listUserNetworks(std::uint64_t peerId) const {
        std::vector<domain::NetworkMembership> result;
        const auto networks = networkRepository_.listNetworks();
        for (const auto& network : networks) {
            const auto vip = membershipStore_.resolveVip(static_cast<std::uint32_t>(network.id), peerId);
            if (vip.has_value()) result.push_back(domain::NetworkMembership{network, *vip});
        }

        return result;
    }

    std::vector<domain::Peer> UserService::listNetworkPeers(std::uint64_t networkId) const {
        if (!networkRepository_.exists(networkId)) {
            return {};
        }

        auto peers = membershipStore_.listPeers(static_cast<std::uint32_t>(networkId));
        for (auto& peer : peers) {
            peer.nickname = peerRepository_.getNickname(peer.peerId).value_or(
                "peer-" + std::to_string(peer.peerId));
        }
        return peers;
    }
}