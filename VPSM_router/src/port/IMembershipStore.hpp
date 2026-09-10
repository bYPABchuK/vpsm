#pragma once

#include "../domain/model/peer.hpp"

#include <cstdint>
#include <optional>
#include <vector>
namespace vpsm::server::port {
    class IMembershipStore {
    public:
        virtual ~IMembershipStore() = default;

        virtual std::optional<std::uint32_t> allocateVip(
            std::uint32_t networkId,
            std::uint64_t peerId
        ) = 0;

        virtual bool releaseVip(
            std::uint32_t networkId,
            std::uint64_t peerId
        ) = 0;
        virtual bool removeNetwork(std::uint32_t networkId) = 0;
        
        virtual bool bindPeer(std::uint32_t networkId, std::uint64_t peerId, std::uint32_t vip) = 0;
        virtual bool unbindPeer(std::uint32_t networkId, std::uint64_t peerId, std::uint32_t vip) = 0;
        virtual bool hasPeer(std::uint32_t networkId, std::uint64_t peerId) const = 0;
        virtual std::optional<std::uint32_t> resolveVip(std::uint32_t networkId, std::uint64_t peerId) const = 0;
        virtual std::optional<std::uint64_t> resolvePeer(std::uint32_t networkId, std::uint32_t vip) const = 0;
        virtual std::vector<domain::Peer> listPeers(std::uint32_t networkId) const = 0;
    };
}