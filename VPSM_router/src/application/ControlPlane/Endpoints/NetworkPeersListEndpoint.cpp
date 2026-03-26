#include "NetworkPeersListEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse NetworkPeersListEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return responseEncoder_.encode(dto::NetworkPeersListResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::NetworkPeersListResultDto{.ok = false, .status = 402, .error = "auth_required"});
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return responseEncoder_.encode(dto::NetworkPeersListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        std::uint64_t networkId = 0;
        try {
            networkId = static_cast<std::uint64_t>(std::stoull(it->second));
        } catch (...) {
            return responseEncoder_.encode(dto::NetworkPeersListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        const auto peers = userService_.listNetworkPeers(networkId);

        dto::NetworkPeersListResultDto result{.ok = true, .status = 200};
        for (const auto& peer : peers) {
            result.peers.push_back(dto::NetworkPeerItemDto{
                .peerId = peer.peerId,
                .vip = peer.vip,
            });
        }

        return responseEncoder_.encode(result);
    }
}
