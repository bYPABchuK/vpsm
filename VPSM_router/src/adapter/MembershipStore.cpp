#include "MembershipStore.hpp"
#include <mutex>
namespace vpsm::server::adapter {
    std::optional<std::uint32_t> MembershipRegistry::allocateVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) {
        std::unique_lock lock(mutex_);
    
        auto& network = networks_[networkId];
    
        if (auto it = network.peersById.find(peerId); it != network.peersById.end()) {
            return it->second.vip;
        }
    
        auto vip = network.allocator.allocate();
        if (!vip.has_value()) {
            return std::nullopt;
        }
    
        domain::Peer peer{
            .peerId = peerId,
            .vip = *vip
        };
    
        network.peersById.emplace(peerId, peer);
        network.peerIdByVip.emplace(*vip, peerId);
    
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
    
        network.allocator.release(vip);
    
        if (network.peersById.empty()) {
            networks_.erase(netIt);
        }
    
        return true;
    }
    
    bool MembershipRegistry::bindPeer(
        std::uint32_t networkId,
        std::uint64_t peerId,
        std::uint32_t vip
    ) {
        std::unique_lock lock(mutex_);
    
        auto& network = networks_[networkId];
    
        auto peerIt = network.peersById.find(peerId);
        if (peerIt != network.peersById.end()) {
            return peerIt->second.vip == vip;
        }
    
        auto vipIt = network.peerIdByVip.find(vip);
        if (vipIt != network.peerIdByVip.end() && vipIt->second != peerId) {
            return false;
        }
    
        if (!network.allocator.reserve(vip)) {
            return false;
        }
    
        domain::Peer peer{
            .peerId = peerId,
            .vip = vip
        };
    
        network.peersById.emplace(peerId, peer);
        network.peerIdByVip.emplace(vip, peerId);
    
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
    
        network.allocator.release(vip);
    
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