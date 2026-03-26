#pragma once

#include "IControlRouter.hpp"
#include "IControlEndpoint.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace vpsm::server::application {
    class ControlRouter final : public IControlRouter {
    public:
        void addRoute(const std::string& method, const std::string& path, std::shared_ptr<IControlEndpoint> endpoint);
        ControlResponse route(const ControlRequest& request) override;

    private:
        static std::string routeKey(const std::string& method, const std::string& path);
        std::unordered_map<std::string, std::shared_ptr<IControlEndpoint>> routes_;
    };
}
