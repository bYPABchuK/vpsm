#pragma once

#include <cstdint>
#include <memory>
#include <vector>
namespace vpsm::server::domain {
    using buffer = std::shared_ptr<std::vector<std::uint8_t>>;
}