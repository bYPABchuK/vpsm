#include "NetworkUserAddEndpoint.hpp"
#include "AuthPolicy.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse NetworkUserAddEndpoint::handle(const ControlRequest& request) {
        if (request.method != "PUT") {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!auth_policy::isAuthenticated(request)) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 402, .error = "auth_required"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto peers = userService_.listNetworkPeers(dto->networkId);
        for (const auto& p : peers) {
            if (p.peerId == *request.authenticatedPeerId) {
                return responseEncoder_.encode(dto::NetworkUserAddResultDto{
                    .ok = true,
                    .status = 208,
                    .vip = p.vip,
                    .alreadyExists = true,
                });
            }
        }

        const auto vip = userService_.joinNetwork(*request.authenticatedPeerId, dto->networkId, dto->passwordHash);
        if (std::holds_alternative<port::UserServiceError>(vip)) {
            const auto error = std::get<port::UserServiceError>(vip);
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }

        const auto joinSuccess = std::get<port::JoinNetworkSuccess>(vip);

        return responseEncoder_.encode(dto::NetworkUserAddResultDto{
            .ok = true,
            .status = 200,
            .vip = joinSuccess.vip,
            .alreadyExists = joinSuccess.alreadyExists,
        });
    }
}
