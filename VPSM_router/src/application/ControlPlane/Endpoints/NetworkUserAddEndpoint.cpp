#include "NetworkUserAddEndpoint.hpp"
#include "AuthPolicy.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse NetworkUserAddEndpoint::handle(const ControlRequest& request) {
        if (request.method != "PUT") {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!auth_policy::isAuthenticated(request)) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 401, .error = "auth_required"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
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
            .status = static_cast<uint16_t>(joinSuccess.alreadyExists ? 208 : 200),
            .networkId = dto->networkId,
            .vip = joinSuccess.vip,
            .networkAddress = joinSuccess.networkAddress,
            .prefixLength = joinSuccess.prefixLength,
            .mtu = joinSuccess.mtu,
            .alreadyExists = joinSuccess.alreadyExists,
        });
    }
}
