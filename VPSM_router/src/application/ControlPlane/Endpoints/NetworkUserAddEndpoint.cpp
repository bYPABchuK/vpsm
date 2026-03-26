#include "NetworkUserAddEndpoint.hpp"

#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application::endpoints {
    ControlResponse NetworkUserAddEndpoint::handle(const ControlRequest& request) {
        if (request.method != "PUT") {
            return ControlResponse{.status = 405, .contentType = "text/plain", .body = {'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'}};
        }
        if (!request.authenticatedPeerId.has_value()) {
            return ControlResponse{.status = 402, .contentType = "text/plain", .body = {'a','u','t','h','_','r','e','q','u','i','r','e','d'}};
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','i','d'}};
        }

        std::uint64_t networkId = 0;
        try { networkId = static_cast<std::uint64_t>(std::stoull(it->second)); }
        catch (...) { return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','i','d'}}; }

        const auto peers = userService_.listNetworkPeers(networkId);
        for (const auto& p : peers) {
            if (p.peerId == *request.authenticatedPeerId) {
                boost::json::object out;
                out["ok"] = true;
                out["alreadyExists"] = true;
                out["vip"] = static_cast<std::int64_t>(p.vip);
                const auto text = boost::json::serialize(out);
                return ControlResponse{.status = 208, .contentType = "application/json", .body = std::vector<std::uint8_t>(text.begin(), text.end())};
            }
        }

        std::string passwordHash;
        if (!request.body.empty()) {
            boost::system::error_code ec;
            const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
            if (ec || !value.is_object()) {
                return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','p','a','y','l','o','a','d'}};
            }
            const auto& obj = value.as_object();
            const auto passIt = obj.find("passwordHash");
            if (passIt != obj.end()) {
                if (!passIt->value().is_string()) {
                    return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','p','a','s','s','w','o','r','d'}};
                }
                passwordHash = std::string(passIt->value().as_string().c_str());
            }
        }

        const auto vip = userService_.joinNetwork(*request.authenticatedPeerId, networkId, passwordHash);
        if (!vip.has_value()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'j','o','i','n','_','f','a','i','l','e','d'}};
        }

        boost::json::object out;
        out["ok"] = true;
        out["vip"] = static_cast<std::int64_t>(*vip);
        out["alreadyExists"] = false;
        const auto text = boost::json::serialize(out);
        return ControlResponse{.status = 200, .contentType = "application/json", .body = std::vector<std::uint8_t>(text.begin(), text.end())};
    }
}
