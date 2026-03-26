#include "JsonCreatePeerResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonCreatePeerResponseEncoder::encode(const dto::CreatePeerResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        if (response.peerId.has_value()) {
            out["peerId"] = static_cast<std::int64_t>(*response.peerId);
        }

        return json_support::toJsonResponse(response.status, out);
    }
}
