#include "JsonJoinNetworkResponseEncoder.hpp"
#include "JsonSupport.hpp"
#include "../../domain/type/overlayNetwork.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonJoinNetworkResponseEncoder::encode(const dto::JoinNetworkResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        if (response.networkId.has_value()) out["networkId"] = std::to_string(*response.networkId);
        if (response.vip.has_value()) {
            const auto address = domain::ipv4ToString(*response.vip);
            out["vip"] = address;
            out["address"] = address;
        }
        if (response.networkAddress.has_value()) out["networkAddress"] = domain::ipv4ToString(*response.networkAddress);
        if (response.prefixLength.has_value()) out["prefixLength"] = *response.prefixLength;
        if (response.mtu.has_value()) out["mtu"] = *response.mtu;

        return json_support::toJsonResponse(response.status, out);
    }
}
