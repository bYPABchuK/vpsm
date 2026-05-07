#include "UiMainBodyEndpoint.hpp"

#include "../JsonSupport.hpp"

#include <initializer_list>
#include <optional>
#include <string>

namespace vpsm::server::application::endpoints {
    namespace {
        std::optional<std::string> headerByName(
            const ControlRequest& request,
            const std::initializer_list<const char*>& names
        ) {
            for (const auto* name : names) {
                const auto it = request.headers.find(name);
                if (it != request.headers.end() && !it->second.empty()) {
                    return it->second;
                }
            }
            return std::nullopt;
        }
    }

    ControlResponse UiMainBodyEndpoint::handle(const ControlRequest& request) {
        if (request.method != "GET") {
            return json_support::toJsonResponse(405, json_support::makeBaseResult(false, std::optional<std::string>{"method_not_allowed"}));
        }

        const auto sessionIdText = headerByName(request, {"sessionId", "SessionId", "X-Session-Id", "x-session-id"});
        const auto sessionKeyText = headerByName(request, {"sessionKey", "SessionKey", "X-Session-Key", "x-session-key"});
        if (!sessionIdText.has_value() || !sessionKeyText.has_value()) {
            return json_support::toJsonResponse(401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"}));
        }

        std::uint64_t sessionId = 0;
        std::uint64_t sessionKey = 0;
        try {
            sessionId = static_cast<std::uint64_t>(std::stoull(*sessionIdText));
            sessionKey = static_cast<std::uint64_t>(std::stoull(*sessionKeyText));
        } catch (...) {
            return json_support::toJsonResponse(401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"}));
        }

        const auto peerId = sessionStore_.authenticate(sessionId, sessionKey);
        if (!peerId.has_value()) {
            return json_support::toJsonResponse(403, json_support::makeBaseResult(false, std::optional<std::string>{"forbidden"}));
        }

        const auto result = uiScreenService_.getMainBody();
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
