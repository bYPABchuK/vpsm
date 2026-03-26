#include "JsonSupport.hpp"

#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application::json_support {
    bool hasJsonContentType(const ControlRequest& request) {
        const auto it = request.headers.find("Content-Type");
        if (it != request.headers.end() && it->second.find("application/json") != std::string::npos) {
            return true;
        }

        const auto lowerIt = request.headers.find("content-type");
        return lowerIt != request.headers.end() && lowerIt->second.find("application/json") != std::string::npos;
    }

    std::optional<boost::json::object> parseJsonObject(const ControlRequest& request) {
        boost::system::error_code ec;
        const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
        if (ec || !value.is_object()) {
            return std::nullopt;
        }

        return value.as_object();
    }

    std::optional<std::string> readString(const boost::json::object& object, const char* key) {
        const auto it = object.find(key);
        if (it == object.end() || !it->value().is_string()) {
            return std::nullopt;
        }

        return std::string(it->value().as_string().c_str());
    }

    std::optional<std::uint64_t> readU64(const boost::json::object& object, const char* key) {
        const auto it = object.find(key);
        if (it == object.end() || !it->value().is_int64() || it->value().as_int64() < 0) {
            return std::nullopt;
        }

        return static_cast<std::uint64_t>(it->value().as_int64());
    }

    std::optional<std::uint64_t> readPathU64(const ControlRequest& request, const char* key) {
        const auto it = request.pathParams.find(key);
        if (it == request.pathParams.end()) {
            return std::nullopt;
        }

        try {
            return static_cast<std::uint64_t>(std::stoull(it->second));
        } catch (...) {
            return std::nullopt;
        }
    }

    boost::json::object makeBaseResult(bool ok, const std::optional<std::string>& error) {
        boost::json::object out;
        out["ok"] = ok;

        if (error.has_value()) {
            out["error"] = *error;
        }

        return out;
    }

    ControlResponse toJsonResponse(int status, const boost::json::object& body) {
        const auto text = boost::json::serialize(body);

        return ControlResponse{
            .status = status,
            .contentType = "application/json",
            .body = std::vector<std::uint8_t>(text.begin(), text.end()),
        };
    }
}