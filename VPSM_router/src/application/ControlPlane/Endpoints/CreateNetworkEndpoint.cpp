#include "CreateNetworkEndpoint.hpp"
#include "AuthPolicy.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse CreateNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        if (!auth_policy::matchesAuthenticatedPeer(request, dto->ownerPeerId)) {
            return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = false, .status = 403, .error = "forbidden"});
        }

        const auto networkId = userService_.createNetwork(dto->ownerPeerId, dto->name, dto->passwordHash);
        if (std::holds_alternative<port::UserServiceError>(networkId)) {
            const auto error = std::get<port::UserServiceError>(networkId);
            return responseEncoder_.encode(dto::CreateNetworkResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }

        return responseEncoder_.encode(dto::CreateNetworkResultDto{.ok = true, .status = 200, .networkId = std::get<port::CreateNetworkSuccess>(networkId).networkId});
    }
}
