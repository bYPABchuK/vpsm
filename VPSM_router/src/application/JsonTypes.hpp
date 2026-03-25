#pragma once

#include <boost/json/value.hpp>

namespace vpsm::server::application {
    struct JsonBodyResponse {
        int status = 200;
        boost::json::value body;
    };
}
