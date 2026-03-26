#include "UserNetworkListEndpoint.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application::endpoints {
    ControlResponse UserNetworkListEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return ControlResponse{.status = 405, .contentType = "text/plain", .body = {'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'}};
        }

        if (!request.authenticatedPeerId.has_value()) {
            return ControlResponse{.status = 402, .contentType = "text/plain", .body = {'a','u','t','h','_','r','e','q','u','i','r','e','d'}};
        }

        const auto it = request.pathParams.find("id");
        if (it == request.pathParams.end()) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','i','d'}};
        }

        std::uint64_t peerId = 0;
        try {
            peerId = static_cast<std::uint64_t>(std::stoull(it->second));
        } catch (...) {
            return ControlResponse{.status = 400, .contentType = "text/plain", .body = {'i','n','v','a','l','i','d','_','i','d'}};
        }

        if (peerId != *request.authenticatedPeerId) {
            return ControlResponse{.status = 403, .contentType = "text/plain", .body = {'f','o','r','b','i','d','d','e','n'}};
        }

        const auto networks = userService_.listUserNetworks(peerId);

        boost::json::object out;
        out["ok"] = true;
        boost::json::array items;
        for (const auto& network : networks) {
            boost::json::object item;
            item["id"] = static_cast<std::int64_t>(network.id);
            item["name"] = network.name;
            item["ownerPeerId"] = static_cast<std::int64_t>(network.owner.peerId);
            items.push_back(std::move(item));
        }
        out["networks"] = std::move(items);

        const auto text = boost::json::serialize(out);
        return ControlResponse{.status = 200, .contentType = "application/json", .body = std::vector<std::uint8_t>(text.begin(), text.end())};
    }
}
