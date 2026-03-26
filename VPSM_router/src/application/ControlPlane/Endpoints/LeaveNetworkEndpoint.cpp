#include "LeaveNetworkEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse LeaveNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != expectedMethod_) {
            return ControlResponse{.status = 405, .contentType = "text/plain", .body = {'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'}};
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto ok = userService_.leaveNetwork(dto->peerId, dto->networkId);
        return responseEncoder_.encode(dto::LeaveNetworkResultDto{.ok = ok, .status = 200});
    }
}
