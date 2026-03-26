#include "LeaveNetworkEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse LeaveNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != expectedMethod_) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto ok = userService_.leaveNetwork(dto->peerId, dto->networkId);
        return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = ok, .status = 200});
    }
}
