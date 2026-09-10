#pragma once

#include <memory>

namespace vpsm::client {
    class ITunDevice;
    class INetworkConfigurator;

    class IPlatformServices {
    public:
        virtual ~IPlatformServices() = default;
        virtual std::unique_ptr<ITunDevice> createTunDevice() = 0;
        virtual std::unique_ptr<INetworkConfigurator> createNetworkConfigurator() = 0;
    };
}