#include "JsonLoginResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonLoginResponseEncoder::encode(const dto::LoginResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        if (response.peerId.has_value()) {
            out["peerId"] = static_cast<std::int64_t>(*response.peerId);
        }
        if (response.sessionId.has_value()) {
            out["sessionId"] = static_cast<std::int64_t>(*response.sessionId);
        }
        if (response.sessionKey.has_value()) {
            out["sessionKey"] = static_cast<std::int64_t>(*response.sessionKey);
        }

        return json_support::toJsonResponse(response.status, out);
    }
}
