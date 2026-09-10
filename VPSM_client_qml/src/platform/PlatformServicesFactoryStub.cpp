#include "PlatformServicesFactory.hpp"

#include "platform/stub/StubPlatformServices.hpp"

namespace vpsm::client {
    std::unique_ptr<IPlatformServices> createPlatformServices() {
        return std::make_unique<StubPlatformServices>();
    }
}