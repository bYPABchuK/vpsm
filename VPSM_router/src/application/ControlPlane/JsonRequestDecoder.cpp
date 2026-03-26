#include "JsonRequestDecoder.hpp"

#include <boost/json/error.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace vpsm::server::application {
    std::optional<boost::json::value> JsonRequestDecoder::decode(const ControlRequest& request) {
        const std::string text(request.body.begin(), request.body.end());

        boost::system::error_code ec;
        auto value = boost::json::parse(text, ec);
        if (ec) {
            return std::nullopt;
        }

        return value;
    }
}
