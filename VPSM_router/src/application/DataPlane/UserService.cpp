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

        const auto networkId = networkRepository_.createNetwork(ownerPeerId, name, passwordHash);
        if (!networkId.has_value()) {
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
            return port::JoinNetworkSuccess{.vip = *vip, .alreadyExists = true};
        }

        const auto vip = membershipStore_.allocateVip(networkId, peerId);
        if (!vip.has_value()) {
            return port::UserServiceError::CreateFailed;
        }

        return port::JoinNetworkSuccess{.vip = *vip, .alreadyExists = false};
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

    std::vector<domain::VNetwork> UserService::listUserNetworks(std::uint64_t peerId) const {
        std::vector<domain::VNetwork> result;
        const auto networks = networkRepository_.listNetworks();
        for (const auto& network : networks) {
            if (membershipStore_.hasPeer(static_cast<std::uint32_t>(network.id), peerId)) {
                result.push_back(network);
            }
        }

        return result;
    }

    std::vector<domain::Peer> UserService::listNetworkPeers(std::uint64_t networkId) const {
        if (!networkRepository_.exists(networkId)) {
            return {};
        }

        return membershipStore_.listPeers(static_cast<std::uint32_t>(networkId));
    }
}