#include "UiLicensescreenEndpoint.hpp"

#include "../JsonSupport.hpp"

#include <optional>

namespace vpsm::server::application::endpoints {
    ControlResponse UiLicensescreenEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return json_support::toJsonResponse(405, json_support::makeBaseResult(false, std::optional<std::string>{"method_not_allowed"}));
        }

        const auto result = uiScreenService_.getLicensescreen();
        if (result.status == UiScreenService::Status::NotFound) {
            return json_support::toJsonResponse(404, json_support::makeBaseResult(false, std::optional<std::string>{"not_supported"}));
        }
        if (result.status == UiScreenService::Status::Error) {
            return json_support::toJsonResponse(500, json_support::makeBaseResult(false, std::optional<std::string>{"internal_error"}));
        }

        auto out = json_support::makeBaseResult(true, std::nullopt);
        const auto screenIt = result.payload.find("screen");
        if (screenIt != result.payload.end() && screenIt->value().is_object()) {
            out["screen"] = screenIt->value().as_object();
        } else {
            out["screen"] = result.payload;
        }
        return json_support::toJsonResponse(200, out);
    }
}
