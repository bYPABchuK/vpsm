#include "JsonCreateNetworkResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonCreateNetworkResponseEncoder::encode(const dto::CreateNetworkResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        if (response.networkId.has_value()) {
            out["networkId"] = static_cast<std::int64_t>(*response.networkId);
        }

        return json_support::toJsonResponse(response.status, out);
    }
}
