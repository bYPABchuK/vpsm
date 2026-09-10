#pragma once

#include "application/ports/IPlatformServices.hpp"

#include <memory>

namespace vpsm::client {
    std::unique_ptr<IPlatformServices> createPlatformServices();
}