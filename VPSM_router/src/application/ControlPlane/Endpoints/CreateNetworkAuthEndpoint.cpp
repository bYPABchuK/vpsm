#include "CreateNetworkAuthEndpoint.hpp"

#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application::endpoints {
    ControlResponse CreateNetworkAuthEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 402, .error = "auth_required"});
        }

        boost::system::error_code ec;
        const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
        if (ec || !value.is_object()) {
            return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        const auto& object = value.as_object();
        const auto nameIt = object.find("name");
        if (nameIt == object.end() || !nameIt->value().is_string()) {
            return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 400, .error = "name_required"});
        }

        std::string password;
        const auto passIt = object.find("passwordHash");
        if (passIt != object.end()) {
            if (!passIt->value().is_string()) {
                return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 400, .error = "invalid_password"});
            }
            password = std::string(passIt->value().as_string().c_str());
        }

        const auto networkId = userService_.createNetwork(
            *request.authenticatedPeerId,
            std::string(nameIt->value().as_string().c_str()),
            password
        );
        if (!networkId.has_value()) {
            return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = false, .status = 400, .error = "create_failed"});
        }

        return responseEncoder_.encode(dto::CreateNetworkAuthResultDto{.ok = true, .status = 200, .networkId = *networkId});
    }
}
