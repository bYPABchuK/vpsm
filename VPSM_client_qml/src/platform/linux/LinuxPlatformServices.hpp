#pragma once

#include "application/ports/IPlatformServices.hpp"

namespace vpsm::client {
    class LinuxPrivilegedHelperClient;

    class LinuxPlatformServices final : public IPlatformServices {
    public:
        std::unique_ptr<ITunDevice> createTunDevice() override;
        std::unique_ptr<INetworkConfigurator> createNetworkConfigurator() override;

    private:
        std::shared_ptr<LinuxPrivilegedHelperClient> helper_;
    };
}