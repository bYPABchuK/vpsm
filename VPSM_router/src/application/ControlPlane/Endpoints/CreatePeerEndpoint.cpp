#include "CreatePeerEndpoint.hpp"
#include "UserServiceErrorMapper.hpp"

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
        if (std::holds_alternative<port::UserServiceError>(peerId)) {
            const auto error = std::get<port::UserServiceError>(peerId);
            return responseEncoder_.encode(dto::CreatePeerResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }

        return responseEncoder_.encode(dto::CreatePeerResultDto{.ok = true, .status = 200, .peerId = std::get<port::CreatePeerSuccess>(peerId).peerId});
    }
}
