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

    std::optional<std::uint64_t> UserService::createPeer(
        const std::string& nickname,
        const std::string& passwordHash
    ) {
        return peerRepository_.createPeer(nickname, passwordHash);
    }

    bool UserService::deletePeer(std::uint64_t peerId) {
        if (!peerRepository_.exists(peerId)) {
            return false;
        }

        return peerRepository_.deletePeer(peerId);
    }

    std::optional<std::uint64_t> UserService::createNetwork(
        std::uint64_t ownerPeerId,
        const std::string& name,
        const std::string& passwordHash
    ) {
        if (!peerRepository_.exists(ownerPeerId)) {
            return std::nullopt;
        }

        return networkRepository_.createNetwork(ownerPeerId, name, passwordHash);
    }

    bool UserService::deleteNetwork(
        std::uint64_t requesterPeerId,
        std::uint64_t networkId
    ) {
        auto network = networkRepository_.getNetwork(networkId);
        if (!network.has_value()) {
            return false;
        }

        if (network->owner.peerId != requesterPeerId) {
            return false;
        }

        return networkRepository_.deleteNetwork(networkId);
    }

    std::optional<std::uint32_t> UserService::joinNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId,
        const std::string& passwordHash
    ) {
        if (!peerRepository_.exists(peerId)) {
            return std::nullopt;
        }

        auto network = networkRepository_.getNetwork(networkId);
        if (!network.has_value()) {
            return std::nullopt;
        }

        if (network->password_hash != passwordHash) {
            return std::nullopt;
        }

        if (membershipStore_.hasPeer(networkId, peerId)) {
            return membershipStore_.resolveVip(networkId, peerId);
        }

        return membershipStore_.allocateVip(networkId, peerId);
    }

    bool UserService::leaveNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId
    ) {
        if (!peerRepository_.exists(peerId)) {
            return false;
        }

        if (!networkRepository_.exists(networkId)) {
            return false;
        }

        if (!membershipStore_.hasPeer(networkId, peerId)) {
            return false;
        }

        return membershipStore_.releaseVip(networkId, peerId);
    }
}