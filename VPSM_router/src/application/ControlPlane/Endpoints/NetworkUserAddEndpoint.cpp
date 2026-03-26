#include "NetworkUserAddEndpoint.hpp"

#include <boost/json/parse.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application::endpoints {
    ControlResponse NetworkUserAddEndpoint::handle(const ControlRequest& request) {
        if (request.method != "PUT") {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }
        if (!request.authenticatedPeerId.has_value()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 402, .error = "auth_required"});
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_id"});
        }

        std::uint64_t networkId = 0;
        try { networkId = static_cast<std::uint64_t>(std::stoull(it->second)); }
        catch (...) { return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_id"}); }

        const auto peers = userService_.listNetworkPeers(networkId);
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

        std::string passwordHash;
        if (!request.body.empty()) {
            boost::system::error_code ec;
            const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
            if (ec || !value.is_object()) {
                return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
            }
            const auto& obj = value.as_object();
            const auto passIt = obj.find("passwordHash");
            if (passIt != obj.end()) {
                if (!passIt->value().is_string()) {
                    return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "invalid_password"});
                }
                passwordHash = std::string(passIt->value().as_string().c_str());
            }
        }

        const auto vip = userService_.joinNetwork(*request.authenticatedPeerId, networkId, passwordHash);
        if (!vip.has_value()) {
            return responseEncoder_.encode(dto::NetworkUserAddResultDto{.ok = false, .status = 400, .error = "join_failed"});
        }

        return responseEncoder_.encode(dto::NetworkUserAddResultDto{
            .ok = true,
            .status = 200,
            .vip = *vip,
            .alreadyExists = false,
        });
    }
}
