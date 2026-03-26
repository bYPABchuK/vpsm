#include "HealthEndpoint.hpp"

namespace vpsm::server::application {
    ControlResponse HealthEndpoint::handle(const ControlRequest& request) {
        (void)request;
        return ControlResponse{
            .status = 200,
            .contentType = "text/plain",
            .body = std::vector<std::uint8_t>{'o', 'k'},
        };
    }
}
