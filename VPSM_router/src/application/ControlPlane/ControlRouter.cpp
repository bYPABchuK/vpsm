#include "ControlRouter.hpp"

#include <utility>

namespace vpsm::server::application {
    void ControlRouter::setAuthenticator(std::function<std::optional<std::uint64_t>(const ControlRequest&)> authenticator) {
        authenticator_ = std::move(authenticator);
    }

    void ControlRouter::addRoute(const std::string& method, const std::string& path, std::shared_ptr<IControlEndpoint> endpoint) {
        auto* node = &root_;
        const auto segments = splitPath(path);

        for (const auto& segment : segments) {
            if (segment.size() >= 3 && segment.front() == '{' && segment.back() == '}') {
                if (!node->paramChild) {
                    node->paramChild = std::make_unique<RouteNode>();
                    node->paramChild->paramName = segment.substr(1, segment.size() - 2);
                }
                node = node->paramChild.get();
                continue;
            }

            auto& child = node->staticChildren[segment];
            if (!child) {
                child = std::make_unique<RouteNode>();
            }
            node = child.get();
        }

        node->handlers[method] = std::move(endpoint);
    }

    ControlResponse ControlRouter::route(const ControlRequest& request) {
        std::optional<std::uint64_t> authenticatedPeerId;
        if (requiresAuth(request.path)) {
            if (!authenticator_) {
                return ControlResponse{
                    .status = 401,
                    .contentType = "text/plain",
                    .body = std::vector<std::uint8_t>{'a','u','t','h','_','r','e','q','u','i','r','e','d'},
                };
            }

            authenticatedPeerId = authenticator_(request);
            if (!authenticatedPeerId.has_value()) {
                return ControlResponse{
                    .status = 401,
                    .contentType = "text/plain",
                    .body = std::vector<std::uint8_t>{'a','u','t','h','_','r','e','q','u','i','r','e','d'},
                };
            }
        }

        const auto segments = splitPath(request.path);
        const auto* node = &root_;
        ControlRequest routedRequest = request;
        routedRequest.pathParams.clear();
        routedRequest.authenticatedPeerId = authenticatedPeerId;

        for (const auto& segment : segments) {
            const auto staticIt = node->staticChildren.find(segment);
            if (staticIt != node->staticChildren.end()) {
                node = staticIt->second.get();
                continue;
            }

            if (node->paramChild) {
                routedRequest.pathParams[node->paramChild->paramName] = segment;
                node = node->paramChild.get();
                continue;
            }

            return ControlResponse{
                .status = 404,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'n', 'o', 't', '_', 'f', 'o', 'u', 'n', 'd'},
            };
        }

        const auto it = node->handlers.find(request.method);
        if (it == node->handlers.end() || !it->second) {
            if (!node->handlers.empty()) {
                return ControlResponse{
                    .status = 405,
                    .contentType = "text/plain",
                    .body = std::vector<std::uint8_t>{'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'},
                };
            }

            return ControlResponse{
                .status = 404,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'n', 'o', 't', '_', 'f', 'o', 'u', 'n', 'd'},
            };
        }

        return it->second->handle(routedRequest);
    }

    std::vector<std::string> ControlRouter::splitPath(const std::string& path) {
        std::vector<std::string> out;
        std::string current;

        const auto endPos = path.find('?');
        const std::size_t limit = (endPos == std::string::npos) ? path.size() : endPos;

        for (std::size_t i = 0; i < limit; ++i) {
            const auto ch = path[i];
            if (ch == '/') {
                if (!current.empty()) {
                    out.push_back(current);
                    current.clear();
                }
                continue;
            }
            current.push_back(ch);
        }

        if (!current.empty()) {
            out.push_back(std::move(current));
        }

        return out;
    }

    bool ControlRouter::requiresAuth(const std::string& path) {
        const auto endPos = path.find('?');
        const std::string normalized = path.substr(0, endPos == std::string::npos ? path.size() : endPos);

        if (normalized == "/user/login") {
            return false;
        }

        return normalized.rfind("/user/", 0) == 0 || normalized.rfind("/network/", 0) == 0;
    }
}
