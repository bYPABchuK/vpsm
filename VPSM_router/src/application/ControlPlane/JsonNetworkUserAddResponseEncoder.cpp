#include "JsonNetworkUserAddResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonNetworkUserAddResponseEncoder::encode(const dto::NetworkUserAddResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);
        out["alreadyExists"] = response.alreadyExists;

        if (response.vip.has_value()) {
            out["vip"] = static_cast<std::int64_t>(*response.vip);
        }

        return json_support::toJsonResponse(response.status, out);
    }
}
