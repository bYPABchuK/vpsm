#include "CreateNetworkAuthEndpoint.hpp"

#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application::endpoints {
    ControlResponse CreateNetworkAuthEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return ControlResponse{.status = 405, .contentType = "text/plain", .body = {'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'}};
        }

        if (!request.authenticatedPeerId.has_value()) {
            return ControlResponse{.status = 402, .contentType = "text/plain", .body = {'a','u','t','h','_','r','e','q','u','i','r','e','d'}};
        }

        boost::system::error_code ec;
        const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
        if (ec || !value.is_object()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','p','a','y','l','o','a','d'}};
        }

        const auto& object = value.as_object();
        const auto nameIt = object.find("name");
        if (nameIt == object.end() || !nameIt->value().is_string()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'n','a','m','e','_','r','e','q','u','i','r','e','d'}};
        }

        std::string password;
        const auto passIt = object.find("passwordHash");
        if (passIt != object.end()) {
            if (!passIt->value().is_string()) {
                return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','p','a','s','s','w','o','r','d'}};
            }
            password = std::string(passIt->value().as_string().c_str());
        }

        const auto networkId = userService_.createNetwork(
            *request.authenticatedPeerId,
            std::string(nameIt->value().as_string().c_str()),
            password
        );
        if (!networkId.has_value()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'c','r','e','a','t','e','_','f','a','i','l','e','d'}};
        }

        boost::json::object out;
        out["ok"] = true;
        out["networkId"] = static_cast<std::int64_t>(*networkId);
        const auto text = boost::json::serialize(out);
        return ControlResponse{.status = 200, .contentType = "application/json", .body = std::vector<std::uint8_t>(text.begin(), text.end())};
    }
}
