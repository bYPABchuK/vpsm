#include "JoinNetworkByNameEndpoint.hpp"
#include "UserServiceErrorMapper.hpp"
#include "../JsonSupport.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse JoinNetworkByNameEndpoint::handle(const ControlRequest& request) {
        if (request.method != "PUT") {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }
        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 401, .error = "auth_required"});
        }
        if (!json_support::hasJsonContentType(request)) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }
        const auto object = json_support::parseJsonObject(request);
        const auto name = object ? json_support::readString(*object, "name") : std::nullopt;
        const auto passwordHash = object ? json_support::readString(*object, "passwordHash") : std::nullopt;
        if (!name || name->empty() || !passwordHash) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto result = userService_.joinNetworkByName(*request.authenticatedPeerId, *name, *passwordHash);
        if (std::holds_alternative<port::UserServiceError>(result)) {
            const auto error = std::get<port::UserServiceError>(result);
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{
                .ok = false,
                .status = user_service_error_mapper::toStatus(error),
                .error = user_service_error_mapper::toErrorString(error),
            });
        }
        const auto joined = std::get<port::JoinNetworkSuccess>(result);
        return responseEncoder_.encode(dto::NetworkUserAddResultDto{
            .ok = true,
            .status = static_cast<std::uint16_t>(joined.alreadyExists ? 208 : 200),
            .networkId = joined.networkId,
            .vip = joined.vip,
            .networkAddress = joined.networkAddress,
            .prefixLength = joined.prefixLength,
            .mtu = joined.mtu,
            .alreadyExists = joined.alreadyExists,
        });
    }
}