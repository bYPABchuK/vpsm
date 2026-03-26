#pragma once

#include "UserDtos.hpp"
#include "../ControlPlane/ControlTypes.hpp"

#include <boost/json/object.hpp>

#include <optional>

namespace vpsm::server::application::dto {
    class JsonDtoDecoder {
    public:
        std::optional<CreatePeerDto> decodeCreatePeer(const boost::json::object& object) const;
        std::optional<CreateNetworkDto> decodeCreateNetwork(const boost::json::object& object) const;
        std::optional<JoinNetworkDto> decodeJoinNetwork(const boost::json::object& object, const ControlRequest& request) const;
        std::optional<LeaveNetworkDto> decodeLeaveNetwork(const boost::json::object& object, const ControlRequest& request) const;
    };
}
