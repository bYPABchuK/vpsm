#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace vpsm::server::port {

class IUserService {
public:
    virtual ~IUserService() = default;

    virtual std::optional<std::uint64_t> createPeer(
        const std::string& nickname,
        const std::string& passwordHash
    ) = 0;

    virtual bool deletePeer(std::uint64_t peerId) = 0;

    virtual std::optional<std::uint64_t> createNetwork(
        std::uint64_t ownerPeerId,
        const std::string& name,
        const std::string& passwordHash
    ) = 0;

    virtual bool deleteNetwork(
        std::uint64_t requesterPeerId,
        std::uint64_t networkId
    ) = 0;

    virtual std::optional<std::uint32_t> joinNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId,
        const std::string& passwordHash
    ) = 0;

    virtual bool leaveNetwork(
        std::uint64_t peerId,
        std::uint64_t networkId
    ) = 0;
};

}