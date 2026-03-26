#pragma once

#include "IControlRouter.hpp"
#include "IControlEndpoint.hpp"

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace vpsm::server::application {
    class ControlRouter final : public IControlRouter {
    public:
        void addRoute(const std::string& method, const std::string& path, std::shared_ptr<IControlEndpoint> endpoint);
        void setAuthenticator(std::function<std::optional<std::uint64_t>(const ControlRequest&)> authenticator);
        ControlResponse route(const ControlRequest& request) override;

    private:
        struct RouteNode {
            std::unordered_map<std::string, std::unique_ptr<RouteNode>> staticChildren;
            std::unique_ptr<RouteNode> paramChild;
            std::string paramName;
            std::unordered_map<std::string, std::shared_ptr<IControlEndpoint>> handlers;
        };

        static std::vector<std::string> splitPath(const std::string& path);
        static bool requiresAuth(const std::string& path);
        RouteNode root_;
        std::function<std::optional<std::uint64_t>(const ControlRequest&)> authenticator_;
    };
}
