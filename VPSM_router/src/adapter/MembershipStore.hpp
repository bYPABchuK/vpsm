#pragma once

#include "../port/IMembershipStore.hpp"
#include "../application/VipAllocator.hpp"
#include "../domain/model/peer.hpp"

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace vpsm::server::adapter {

class MembershipRegistry final : public port::IMembershipStore {
public:
    std::optional<std::uint32_t> allocateVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) override;

    bool releaseVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) override;

    bool bindPeer(
        std::uint32_t networkId,
        std::uint64_t peerId,
        std::uint32_t vip
    ) override;

    bool unbindPeer(
        std::uint32_t networkId,
        std::uint64_t peerId,
        std::uint32_t vip
    ) override;

    bool hasPeer(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) const override;

    std::optional<std::uint32_t> resolveVip(
        std::uint32_t networkId,
        std::uint64_t peerId
    ) const override;

    std::optional<std::uint64_t> resolvePeer(
        std::uint32_t networkId,
        std::uint32_t vip
    ) const override;

private:
    struct NetworkState {
        std::unordered_map<std::uint64_t, domain::Peer> peersById;
        std::unordered_map<std::uint32_t, std::uint64_t> peerIdByVip;
        application::VipAllocator allocator;
    };

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::uint32_t, NetworkState> networks_;
};

}