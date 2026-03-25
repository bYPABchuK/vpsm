#include "ControlRouter.hpp"

namespace vpsm::server::application {
    void ControlRouter::addRoute(const std::string& method, const std::string& path, std::shared_ptr<IControlEndpoint> endpoint) {
        routes_[routeKey(method, path)] = std::move(endpoint);
    }

    ControlResponse ControlRouter::route(const ControlRequest& request) {
        const auto it = routes_.find(routeKey(request.method, request.path));
        if (it == routes_.end() || !it->second) {
            return ControlResponse{
                .status = 404,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'n', 'o', 't', '_', 'f', 'o', 'u', 'n', 'd'},
            };
        }

        return it->second->handle(request);
    }

    std::string ControlRouter::routeKey(const std::string& method, const std::string& path) {
        return method + " " + path;
    }
}
