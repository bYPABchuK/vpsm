#include "DeleteNetworkEndpoint.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse DeleteNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != "DELETE") {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }
        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 401, .error = "auth_required"});
        }
        const auto it = request.pathParams.find("networkId");
        if (it == request.pathParams.end()) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }
        std::uint64_t networkId = 0;
        try { networkId = std::stoull(it->second); }
        catch (...) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }
        const auto result = userService_.deleteNetwork(*request.authenticatedPeerId, networkId);
        if (std::holds_alternative<port::UserServiceError>(result)) {
            const auto error = std::get<port::UserServiceError>(result);
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }
        return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = true, .status = 200});
    }
}