#include "LinuxPlatformServices.hpp"

#include "platform/linux/LinuxNetworkConfigurator.hpp"
#include "platform/linux/LinuxTunDevice.hpp"
#include "platform/linux/LinuxPrivilegedHelperClient.hpp"

namespace vpsm::client {
    std::unique_ptr<ITunDevice> LinuxPlatformServices::createTunDevice() {
        if (!helper_) helper_ = std::make_shared<LinuxPrivilegedHelperClient>();
        return std::make_unique<LinuxTunDevice>(helper_);
    }

    std::unique_ptr<INetworkConfigurator> LinuxPlatformServices::createNetworkConfigurator() {
        if (!helper_) helper_ = std::make_shared<LinuxPrivilegedHelperClient>();
        return std::make_unique<LinuxNetworkConfigurator>(helper_);
    }

}