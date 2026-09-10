#include "MembershipStore.hpp"
#include "../domain/type/overlayNetwork.hpp"
#include <mutex>
namespace vpsm::server::adapter {
    std::optional<std::uint32_t> MembershipRegistry::allocateVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) {
        std::unique_lock lock(mutex_);
    
        const auto overlay = domain::overlayConfigForNetworkId(networkId);
        if (!overlay.has_value()) return std::nullopt;
        auto [networkIt, _] = networks_.try_emplace(networkId);
        auto& network = networkIt->second;
    
        if (auto it = network.peersById.find(peerId); it != network.peersById.end()) {
            return it->second.vip;
        }
    
        auto globalVip = vipByPeerId_.find(peerId);
        std::optional<std::uint32_t> vip;
        if (globalVip != vipByPeerId_.end()) vip = globalVip->second;
        else vip = allocator_.allocate();
        if (!vip.has_value()) return std::nullopt;
    
        domain::Peer peer{
            .peerId = peerId,
            .vip = *vip
        };
    
        network.peersById.emplace(peerId, peer);
        network.peerIdByVip.emplace(*vip, peerId);
        if (globalVip == vipByPeerId_.end()) {
            vipByPeerId_.emplace(peerId, *vip);
            peerIdByVip_.emplace(*vip, peerId);
        }
        ++membershipCountByPeerId_[peerId];
    
        return vip;
    }
    
    bool MembershipRegistry::releaseVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) {
        std::unique_lock lock(mutex_);
    
        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return false;
        }
    
        auto& network = netIt->second;
        auto peerIt = network.peersById.find(peerId);
        if (peerIt == network.peersById.end()) {
            return false;
        }
    
        const auto vip = peerIt->second.vip;
    
        network.peersById.erase(peerIt);
    
        auto vipIt = network.peerIdByVip.find(vip);
        if (vipIt != network.peerIdByVip.end() && vipIt->second == peerId) {
            network.peerIdByVip.erase(vipIt);
        }
    
        auto countIt = membershipCountByPeerId_.find(peerId);
        if (countIt != membershipCountByPeerId_.end() && --countIt->second == 0) {
            membershipCountByPeerId_.erase(countIt);
            vipByPeerId_.erase(peerId);
            peerIdByVip_.erase(vip);
            allocator_.release(vip);
        }
    
        if (network.peersById.empty()) {
            networks_.erase(netIt);
        }
    
        return true;
    }

    bool MembershipRegistry::removeNetwork(std::uint32_t networkId) {
        std::unique_lock lock(mutex_);
        auto networkIt = networks_.find(networkId);
        if (networkIt == networks_.end()) return false;
        for (const auto& [peerId, peer] : networkIt->second.peersById) {
            auto countIt = membershipCountByPeerId_.find(peerId);
            if (countIt != membershipCountByPeerId_.end() && --countIt->second == 0) {
                membershipCountByPeerId_.erase(countIt);
                vipByPeerId_.erase(peerId);
                peerIdByVip_.erase(peer.vip);
                allocator_.release(peer.vip);
            }
        }
        networks_.erase(networkIt);
        return true;
    }
    
    bool MembershipRegistry::bindPeer(
        std::uint32_t networkId,
        std::uint64_t peerId,
        std::uint32_t vip
    ) {
        std::unique_lock lock(mutex_);
    
        const auto overlay = domain::overlayConfigForNetworkId(networkId);
        if (!overlay.has_value() || vip < overlay->firstHost || vip > overlay->lastHost) return false;
        auto [networkIt, _] = networks_.try_emplace(networkId);
        auto& network = networkIt->second;
    
        auto peerIt = network.peersById.find(peerId);
        if (peerIt != network.peersById.end()) {
            return peerIt->second.vip == vip;
        }
    
        auto globalPeer = peerIdByVip_.find(vip);
        if (globalPeer != peerIdByVip_.end() && globalPeer->second != peerId) return false;
        auto globalVip = vipByPeerId_.find(peerId);
        if (globalVip != vipByPeerId_.end() && globalVip->second != vip) return false;
        if (globalVip == vipByPeerId_.end() && !allocator_.reserve(vip)) return false;
    
        domain::Peer peer{
            .peerId = peerId,
            .vip = vip
        };
    
        network.peersById.emplace(peerId, peer);
        network.peerIdByVip.emplace(vip, peerId);
        if (globalVip == vipByPeerId_.end()) {
            vipByPeerId_.emplace(peerId, vip);
            peerIdByVip_.emplace(vip, peerId);
        }
        ++membershipCountByPeerId_[peerId];
    
        return true;
    }
    
    bool MembershipRegistry::unbindPeer(
        std::uint32_t networkId,
        std::uint64_t peerId,
        std::uint32_t vip
    ) {
        std::unique_lock lock(mutex_);
    
        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return false;
        }
    
        auto& network = netIt->second;
        auto peerIt = network.peersById.find(peerId);
        if (peerIt == network.peersById.end() || peerIt->second.vip != vip) {
            return false;
        }
    
        network.peersById.erase(peerIt);
    
        auto vipIt = network.peerIdByVip.find(vip);
        if (vipIt != network.peerIdByVip.end() && vipIt->second == peerId) {
            network.peerIdByVip.erase(vipIt);
        }
    
        auto countIt = membershipCountByPeerId_.find(peerId);
        if (countIt != membershipCountByPeerId_.end() && --countIt->second == 0) {
            membershipCountByPeerId_.erase(countIt);
            vipByPeerId_.erase(peerId);
            peerIdByVip_.erase(vip);
            allocator_.release(vip);
        }
    
        if (network.peersById.empty()) {
            networks_.erase(netIt);
        }
    
        return true;
    }
    
    bool MembershipRegistry::hasPeer(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) const {
        std::shared_lock lock(mutex_);
    
        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return false;
        }
    
        return netIt->second.peersById.find(peerId) != netIt->second.peersById.end();
    }
    
    std::optional<std::uint32_t> MembershipRegistry::resolveVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) const {
        std::shared_lock lock(mutex_);
    
        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return std::nullopt;
        }
    
        auto peerIt = netIt->second.peersById.find(peerId);
        if (peerIt == netIt->second.peersById.end()) {
            return std::nullopt;
        }
    
        return peerIt->second.vip;
    }
    
    std::optional<std::uint64_t> MembershipRegistry::resolvePeer(
        std::uint32_t networkId,
        std::uint32_t vip
    ) const {
        std::shared_lock lock(mutex_);
    
        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return std::nullopt;
        }
    
        auto vipIt = netIt->second.peerIdByVip.find(vip);
        if (vipIt == netIt->second.peerIdByVip.end()) {
            return std::nullopt;
        }
    
        return vipIt->second;
    }

    std::vector<domain::Peer> MembershipRegistry::listPeers(
        std::uint32_t networkId
    ) const {
        std::shared_lock lock(mutex_);

        auto netIt = networks_.find(networkId);
        if (netIt == networks_.end()) {
            return {};
        }

        std::vector<domain::Peer> out;
        out.reserve(netIt->second.peersById.size());
        for (const auto& [_, peer] : netIt->second.peersById) {
            out.push_back(peer);
        }

        return out;
    }
}