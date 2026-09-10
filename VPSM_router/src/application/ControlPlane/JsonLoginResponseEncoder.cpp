#include "JsonLoginResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonLoginResponseEncoder::encode(const dto::LoginResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        if (response.peerId.has_value()) {
            out["peerId"] = std::to_string(*response.peerId);
        }
        if (response.sessionId.has_value()) {
            out["sessionId"] = std::to_string(*response.sessionId);
        }
        if (response.sessionKey.has_value()) {
            out["sessionKey"] = std::to_string(*response.sessionKey);
        }
        if (response.dataPlaneKey.has_value()) {
            out["dataPlaneKey"] = *response.dataPlaneKey;
        }

        return json_support::toJsonResponse(response.status, out);
    }
}
