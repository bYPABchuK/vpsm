#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vpsm::server::application {
    struct ControlRequest {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::vector<std::uint8_t> body;
    };

    struct ControlResponse {
        int status = 500;
        std::string contentType = "text/plain";
        std::vector<std::uint8_t> body;
    };
}
