#include "JsonLeaveNetworkResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonLeaveNetworkResponseEncoder::encode(const dto::LeaveNetworkResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);
        return json_support::toJsonResponse(response.status, out);
    }
}
