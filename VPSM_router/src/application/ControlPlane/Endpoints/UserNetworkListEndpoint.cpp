#include "UserNetworkListEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse UserNetworkListEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return responseEncoder_.encode(dto::UserNetworkListResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::UserNetworkListResultDto{.ok = false, .status = 401, .error = "auth_required"});
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return responseEncoder_.encode(dto::UserNetworkListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        std::uint64_t peerId = 0;
        try {
            peerId = static_cast<std::uint64_t>(std::stoull(it->second));
        } catch (...) {
            return responseEncoder_.encode(dto::UserNetworkListResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        if (peerId != *request.authenticatedPeerId) {
            return responseEncoder_.encode(dto::UserNetworkListResultDto{.ok = false, .status = 403, .error = "forbidden"});
        }

        const auto networks = userService_.listUserNetworks(peerId);

        dto::UserNetworkListResultDto result{
            .ok = true,
            .status = 200,
        };
        for (const auto& membership : networks) {
            const auto& network = membership.network;
            result.networks.push_back(dto::UserNetworkItemDto{
                .id = network.id,
                .name = network.name,
                .ownerPeerId = network.owner.peerId,
                .localVip = membership.localVip,
                .networkAddress = network.networkAddress,
                .prefixLength = network.prefixLength,
                .mtu = network.mtu,
            });
        }

        return responseEncoder_.encode(result);
    }
}
