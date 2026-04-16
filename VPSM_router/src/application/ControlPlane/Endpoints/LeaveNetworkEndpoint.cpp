#include "LeaveNetworkEndpoint.hpp"
#include "AuthPolicy.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse LeaveNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != expectedMethod_) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        if (!auth_policy::matchesAuthenticatedPeer(request, dto->peerId)) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 403, .error = "forbidden"});
        }

        const auto ok = userService_.leaveNetwork(dto->peerId, dto->networkId);
        if (std::holds_alternative<port::UserServiceError>(ok)) {
            const auto error = std::get<port::UserServiceError>(ok);
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }

        return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = true, .status = 200});
    }
}
