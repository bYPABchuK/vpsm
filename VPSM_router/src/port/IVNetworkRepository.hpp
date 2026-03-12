#pragma once

#include "../domain/model/vnetwork.hpp"
#include <cstdint>
#include <optional>
#include <string>
namespace vpsm::server::port {
    class IVNetworkRepository {
    public:
        virtual ~IVNetworkRepository() = default;

        virtual std::optional<std::uint64_t> createNetwork(
            std::uint64_t ownerPeerId,
            const std::string& name,
            const std::string& passwordHash
        ) = 0;

        virtual bool deleteNetwork(std::uint64_t networkId) = 0;

        virtual std::optional<domain::VNetwork> getNetwork(std::uint64_t networkId) const = 0;

        virtual bool exists(std::uint64_t networkId) const = 0;
    };
}