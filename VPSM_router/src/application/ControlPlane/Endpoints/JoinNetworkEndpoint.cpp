#include "JoinNetworkEndpoint.hpp"
#include "AuthPolicy.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse JoinNetworkEndpoint::handle(const ControlRequest& request) {
        if (request.method != expectedMethod_) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        if (!auth_policy::matchesAuthenticatedPeer(request, dto->peerId)) {
            return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = false, .status = 403, .error = "forbidden"});
        }

        const auto vip = userService_.joinNetwork(dto->peerId, dto->networkId, dto->passwordHash);
        if (std::holds_alternative<port::UserServiceError>(vip)) {
            const auto error = std::get<port::UserServiceError>(vip);
            return responseEncoder_.encode(dto::JoinNetworkResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }

        return responseEncoder_.encode(dto::JoinNetworkResultDto{.ok = true, .status = 200, .vip = std::get<port::JoinNetworkSuccess>(vip).vip});
    }
}
