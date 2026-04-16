#pragma once

#include "../../../port/UserServiceResult.hpp"

#include <cstdint>
#include <string>

namespace vpsm::server::application::endpoints::user_service_error_mapper {
    inline std::string toErrorString(port::UserServiceError error) {
        switch (error) {
            case port::UserServiceError::PeerNotFound:
                return "peer_not_found";
            case port::UserServiceError::NetworkNotFound:
                return "network_not_found";
            case port::UserServiceError::NetworkNameAlreadyExists:
                return "network_name_already_exists";
            case port::UserServiceError::InvalidPassword:
                return "invalid_password";
            case port::UserServiceError::AlreadyMember:
                return "already_member";
            case port::UserServiceError::NotMember:
                return "not_member";
            case port::UserServiceError::Forbidden:
                return "forbidden";
            case port::UserServiceError::CreateFailed:
                return "create_failed";
            case port::UserServiceError::DeleteFailed:
                return "delete_failed";
            case port::UserServiceError::InternalError:
                return "internal_error";
        }

        return "internal_error";
    }

    inline std::uint16_t toStatus(port::UserServiceError error) {
        switch (error) {
            case port::UserServiceError::InvalidPassword:
            case port::UserServiceError::Forbidden:
                return 403;
            case port::UserServiceError::InternalError:
                return 500;
            case port::UserServiceError::NetworkNameAlreadyExists:
                return 409;
            default:
                return 400;
        }
    }
}