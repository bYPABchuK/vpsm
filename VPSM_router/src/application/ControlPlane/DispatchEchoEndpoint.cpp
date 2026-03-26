#include "DispatchEchoEndpoint.hpp"

#include <boost/json/object.hpp>

namespace vpsm::server::application {
    namespace {
        bool isJsonContentType(const ControlRequest& request) {
            const auto it = request.headers.find("Content-Type");
            if (it != request.headers.end() && it->second.find("application/json") != std::string::npos) {
                return true;
            }

            const auto lowerIt = request.headers.find("content-type");
            return lowerIt != request.headers.end() && lowerIt->second.find("application/json") != std::string::npos;
        }
    }

    ControlResponse DispatchEchoEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return ControlResponse{
                .status = 405,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'m', 'e', 't', 'h', 'o', 'd', '_', 'n', 'o', 't', '_', 'a', 'l', 'l', 'o', 'w', 'e', 'd'},
            };
        }

        if (!isJsonContentType(request)) {
            return ControlResponse{
                .status = 415,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'u', 'n', 's', 'u', 'p', 'p', 'o', 'r', 't', 'e', 'd', '_', 'm', 'e', 'd', 'i', 'a', '_', 't', 'y', 'p', 'e'},
            };
        }

        const auto payload = decoder_.decode(request);
        if (!payload.has_value()) {
            return ControlResponse{
                .status = 400,
                .contentType = "text/plain",
                .body = std::vector<std::uint8_t>{'i', 'n', 'v', 'a', 'l', 'i', 'd', '_', 'j', 's', 'o', 'n'},
            };
        }

        boost::json::object out;
        out["ok"] = true;
        out["echo"] = *payload;

        return encoder_.encode(JsonBodyResponse{
            .status = 200,
            .body = out,
        });
    }
}
