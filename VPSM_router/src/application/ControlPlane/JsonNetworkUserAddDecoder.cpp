#include "JsonNetworkUserAddDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::NetworkUserAddDto> JsonNetworkUserAddDecoder::decode(const ControlRequest& request) {
        const auto networkId = json_support::readPathU64(request, "id");
        if (!networkId.has_value()) {
            return std::nullopt;
        }

        std::string passwordHash;
        if (!request.body.empty()) {
            if (!json_support::hasJsonContentType(request)) {
                return std::nullopt;
            }

            const auto object = json_support::parseJsonObject(request);
            if (!object.has_value()) {
                return std::nullopt;
            }

            const auto passIt = object->find("passwordHash");
            if (passIt != object->end() && !passIt->value().is_string()) {
                return std::nullopt;
            }

            if (const auto password = json_support::readString(*object, "passwordHash")) {
                passwordHash = *password;
            }
        }

        return dto::NetworkUserAddDto{
            .networkId = *networkId,
            .passwordHash = std::move(passwordHash),
        };
    }
}