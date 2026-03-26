#include "CreateNetworkEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse CreateNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto networkId = userService_.createNetwork(dto->ownerPeerId, dto->name, dto->passwordHash);
        if (!networkId.has_value()) {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 400});
        }

        return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = true, .status = 200, .networkId = *networkId});
    }
}
