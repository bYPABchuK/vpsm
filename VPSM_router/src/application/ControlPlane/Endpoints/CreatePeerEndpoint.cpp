#include "CreatePeerEndpoint.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse CreatePeerEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return responseEncoder_.encode(dto::CreatePeerResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::CreatePeerResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto peerId = userService_.createPeer(dto->nickname, dto->passwordHash);
        if (!peerId.has_value()) {
            return responseEncoder_.encode(dto::CreatePeerResultDto{.ok = false, .status = 400});
        }

        return responseEncoder_.encode(dto::CreatePeerResultDto{.ok = true, .status = 200, .peerId = *peerId});
    }
}
