#pragma once

#include "../port/IVNetworkRepository.hpp"

#include <cstdint>
#include <shared_mutex>
#include <unordered_map>

namespace vpsm::server::repository {
    class InMemoryVNetworkRepository final : public port::IVNetworkRepository {
    public:
        std::optional<std::uint64_t> createNetwork(
            std::uint64_t ownerPeerId,
            const std::string& name,
            const std::string& passwordHash
        ) override;

        bool deleteNetwork(std::uint64_t networkId) override;
        std::optional<domain::VNetwork> getNetwork(std::uint64_t networkId) const override;
        std::vector<domain::VNetwork> listNetworks() const override;
        bool exists(std::uint64_t networkId) const override;

    private:
        mutable std::shared_mutex mutex_;
        std::uint64_t nextId_ = 1;
        std::unordered_map<std::uint64_t, domain::VNetwork> networks_;
    };
}
