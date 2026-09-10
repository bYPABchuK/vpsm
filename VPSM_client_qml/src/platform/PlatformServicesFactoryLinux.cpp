#include "PlatformServicesFactory.hpp"

#include "platform/linux/LinuxPlatformServices.hpp"

namespace vpsm::client {
    std::unique_ptr<IPlatformServices> createPlatformServices() {
        return std::make_unique<LinuxPlatformServices>();
    }
}