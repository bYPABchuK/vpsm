#pragma once

#include "../ControlTypes.hpp"

namespace vpsm::server::application::endpoints::auth_policy {
    inline bool isAuthenticated(const ControlRequest& request) {
        return request.authenticatedPeerId.has_value();
    }

    inline bool matchesAuthenticatedPeer(const ControlRequest& request, std::uint64_t peerId) {
        return request.authenticatedPeerId.has_value() && *request.authenticatedPeerId == peerId;
    }
}