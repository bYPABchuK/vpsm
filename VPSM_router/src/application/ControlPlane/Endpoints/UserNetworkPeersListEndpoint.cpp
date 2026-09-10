#include "UserNetworkPeersListEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse UserNetworkPeersListEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return responseEncoder_.encode(dto::UserNetworkPeersListResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::UserNetworkPeersListResultDto{.ok = false, .status = 401, .error = "auth_required"});
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return responseEncoder_.encode(dto::UserNetworkPeersListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        std::uint64_t peerId = 0;
        try {
            peerId = static_cast<std::uint64_t>(std::stoull(it->second));
        } catch (...) {
            return responseEncoder_.encode(dto::UserNetworkPeersListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        if (peerId != *request.authenticatedPeerId) {
            return responseEncoder_.encode(dto::UserNetworkPeersListResultDto{.ok = false, .status = 403, .error = "forbidden"});
        }

        const auto networks = userService_.listUserNetworks(peerId);

        dto::UserNetworkPeersListResultDto result{
            .ok = true,
            .status = 200,
        };

        for (const auto& membership : networks) {
            const auto& network = membership.network;
            dto::UserNetworkWithPeersItemDto networkItem{
                .id = network.id,
                .name = network.name,
                .ownerPeerId = network.owner.peerId,
                .localVip = membership.localVip,
                .networkAddress = network.networkAddress,
                .prefixLength = network.prefixLength,
                .mtu = network.mtu,
            };

            const auto peers = userService_.listNetworkPeers(network.id);
            for (const auto& peer : peers) {
                networkItem.peers.push_back(dto::NetworkPeerItemDto{
                    .peerId = peer.peerId,
                    .vip = peer.vip,
                    .nickname = peer.nickname,
                });
            }

            result.networks.push_back(std::move(networkItem));
        }

        return responseEncoder_.encode(result);
    }
}