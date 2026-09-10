#include "InMemoryVNetworkRepository.hpp"
#include "../domain/type/overlayNetwork.hpp"

#include <mutex>

namespace vpsm::server::repository {
    std::optional<std::uint64_t> InMemoryVNetworkRepository::createNetwork(
        std::uint64_t ownerPeerId,
        const std::string& name,
        const std::string& passwordHash
    ) {
        std::unique_lock lock(mutex_);

        for (const auto& [_, network] : networks_) {
            if (network.name == name) return std::nullopt;
        }

        std::uint64_t id = 0;
        if (!freeIds_.empty()) {
            id = freeIds_.top();
            freeIds_.pop();
        } else {
            if (nextId_ > 255) return std::nullopt;
            id = nextId_++;
        }
        const auto overlay = domain::overlayConfigForNetworkId(id);
        if (!overlay.has_value()) return std::nullopt;

        domain::VNetwork network;
        network.id = id;
        network.name = name;
        network.password_hash = passwordHash;
        network.networkAddress = overlay->networkAddress;
        network.prefixLength = overlay->prefixLength;
        network.mtu = overlay->mtu;
        network.owner.peerId = ownerPeerId;
        network.owner.vip = 0;

        networks_[id] = network;
        return id;
    }

    bool InMemoryVNetworkRepository::deleteNetwork(std::uint64_t networkId) {
        std::unique_lock lock(mutex_);
        if (networks_.erase(networkId) == 0) return false;
        freeIds_.push(networkId);
        return true;
    }

    std::optional<domain::VNetwork> InMemoryVNetworkRepository::getNetwork(std::uint64_t networkId) const {
        std::shared_lock lock(mutex_);

        const auto it = networks_.find(networkId);
        if (it == networks_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    std::optional<domain::VNetwork> InMemoryVNetworkRepository::getNetworkByName(const std::string& name) const {
        std::shared_lock lock(mutex_);
        for (const auto& [_, network] : networks_) {
            if (network.name == name) return network;
        }
        return std::nullopt;
    }

    std::vector<domain::VNetwork> InMemoryVNetworkRepository::listNetworks() const {
        std::shared_lock lock(mutex_);

        std::vector<domain::VNetwork> out;
        out.reserve(networks_.size());
        for (const auto& [_, network] : networks_) {
            out.push_back(network);
        }

        return out;
    }

    bool InMemoryVNetworkRepository::exists(std::uint64_t networkId) const {
        std::shared_lock lock(mutex_);
        return networks_.find(networkId) != networks_.end();
    }

    bool InMemoryVNetworkRepository::existsByName(const std::string& name) const {
        std::shared_lock lock(mutex_);

        for (const auto& [_, network] : networks_) {
            if (network.name == name) {
                return true;
            }
        }

        return false;
    }
}
