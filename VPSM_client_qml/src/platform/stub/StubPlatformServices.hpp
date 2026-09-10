#pragma once

#include "application/ports/IPlatformServices.hpp"

namespace vpsm::client {
    class StubPlatformServices final : public IPlatformServices {
    public:
        std::unique_ptr<ITunDevice> createTunDevice() override;
        std::unique_ptr<INetworkConfigurator> createNetworkConfigurator() override;
    };
}