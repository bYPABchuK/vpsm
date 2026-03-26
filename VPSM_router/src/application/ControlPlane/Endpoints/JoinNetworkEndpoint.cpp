#include "JoinNetworkEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse JoinNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != expectedMethod_) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto vip = userService_.joinNetwork(dto->peerId, dto->networkId, dto->passwordHash);
        if (!vip.has_value()) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 400});
        }

        return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = true, .status = 200, .vip = *vip});
    }
}
