#include "JsonUserNetworkPeersListResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonUserNetworkPeersListResponseEncoder::encode(const dto::UserNetworkPeersListResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        boost::json::array networks;
        for (const auto& network : response.networks) {
            boost::json::object networkItem;
            networkItem["id"] = static_cast<std::int64_t>(network.id);
            networkItem["name"] = network.name;
            networkItem["ownerPeerId"] = static_cast<std::int64_t>(network.ownerPeerId);

            boost::json::array peers;
            for (const auto& peer : network.peers) {
                boost::json::object peerItem;
                peerItem["peerId"] = static_cast<std::int64_t>(peer.peerId);
                peerItem["vip"] = static_cast<std::int64_t>(peer.vip);
                peers.push_back(std::move(peerItem));
            }
            networkItem["peers"] = std::move(peers);

            networks.push_back(std::move(networkItem));
        }

        out["networks"] = std::move(networks);
        return json_support::toJsonResponse(response.status, out);
    }
}