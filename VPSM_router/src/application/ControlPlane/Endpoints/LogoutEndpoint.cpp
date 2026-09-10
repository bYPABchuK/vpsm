#include "LogoutEndpoint.hpp"

#include "../JsonSupport.hpp"

#include <initializer_list>
#include <optional>
#include <string>

namespace vpsm::server::application::endpoints {
    namespace {
        std::optional<std::string> sessionIdHeader(const ControlRequest& request) {
            for (const auto* name : {"sessionId", "SessionId", "X-Session-Id", "x-session-id"}) {
                const auto it = request.headers.find(name);
                if (it != request.headers.end() && !it->second.empty()) return it->second;
            }
            return std::nullopt;
        }
    }

    ControlResponse LogoutEndpoint::handle(const ControlRequest& request) {
        if (request.method != "DELETE") {
            return json_support::toJsonResponse(
                405, json_support::makeBaseResult(false, std::optional<std::string>{"method_not_allowed"})
            );
        }
        if (!request.authenticatedPeerId.has_value()) {
            return json_support::toJsonResponse(
                401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"})
            );
        }
        const auto text = sessionIdHeader(request);
        if (!text.has_value()) {
            return json_support::toJsonResponse(
                401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"})
            );
        }
        std::uint64_t sessionId = 0;
        try {
            sessionId = static_cast<std::uint64_t>(std::stoull(*text));
        } catch (...) {
            return json_support::toJsonResponse(
                401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"})
            );
        }
        if (!sessionStore_.revoke(sessionId)) {
            return json_support::toJsonResponse(
                401, json_support::makeBaseResult(false, std::optional<std::string>{"auth_required"})
            );
        }
        return json_support::toJsonResponse(200, json_support::makeBaseResult(true, std::nullopt));
    }
}