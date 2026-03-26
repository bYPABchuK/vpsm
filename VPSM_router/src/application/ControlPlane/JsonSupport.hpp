#pragma once

#include "ControlTypes.hpp"

#include <boost/json/object.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace vpsm::server::application::json_support {
    bool hasJsonContentType(const ControlRequest& request);

    std::optional<boost::json::object> parseJsonObject(const ControlRequest& request);

    std::optional<std::string> readString(const boost::json::object& object, const char* key);
    std::optional<std::uint64_t> readU64(const boost::json::object& object, const char* key);
    std::optional<std::uint64_t> readPathU64(const ControlRequest& request, const char* key);

    boost::json::object makeBaseResult(bool ok, const std::optional<std::string>& error);
    ControlResponse toJsonResponse(int status, const boost::json::object& body);
}