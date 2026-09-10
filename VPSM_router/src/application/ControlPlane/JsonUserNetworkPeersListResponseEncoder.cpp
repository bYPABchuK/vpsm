#include "JsonUserNetworkPeersListResponseEncoder.hpp"
#include "JsonSupport.hpp"
#include "../../domain/type/overlayNetwork.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonUserNetworkPeersListResponseEncoder::encode(const dto::UserNetworkPeersListResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        boost::json::array networks;
        for (const auto& network : response.networks) {
            boost::json::object networkItem;
            networkItem["id"] = std::to_string(network.id);
            networkItem["name"] = network.name;
            networkItem["ownerPeerId"] = std::to_string(network.ownerPeerId);
            networkItem["address"] = domain::ipv4ToString(network.localVip);
            networkItem["networkAddress"] = domain::ipv4ToString(network.networkAddress);
            networkItem["prefixLength"] = network.prefixLength;
            networkItem["mtu"] = network.mtu;

            boost::json::array peers;
            for (const auto& peer : network.peers) {
                boost::json::object peerItem;
                peerItem["peerId"] = std::to_string(peer.peerId);
                peerItem["vip"] = domain::ipv4ToString(peer.vip);
                peerItem["nickname"] = peer.nickname;
                peers.push_back(std::move(peerItem));
            }
            networkItem["peers"] = std::move(peers);

            networks.push_back(std::move(networkItem));
        }

        out["networks"] = std::move(networks);
        return json_support::toJsonResponse(response.status, out);
    }
}