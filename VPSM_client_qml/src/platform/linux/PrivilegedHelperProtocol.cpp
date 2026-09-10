#include "PrivilegedHelperProtocol.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace vpsm::client::helper_protocol {
    namespace {
        template<typename T>
        bool receiveMessage(int socket, T& value, int* descriptor) {
            iovec io{&value, sizeof(value)};
            alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int))]{};
            msghdr message{};
            message.msg_iov = &io;
            message.msg_iovlen = 1;
            if (descriptor) {
                message.msg_control = control;
                message.msg_controllen = sizeof(control);
                *descriptor = -1;
            }
            ssize_t received;
            do { received = ::recvmsg(socket, &message, 0); }
            while (received < 0 && errno == EINTR);
            if (received != static_cast<ssize_t>(sizeof(value)) || (message.msg_flags & (MSG_TRUNC | MSG_CTRUNC)))
                return false;
            if (descriptor) {
                for (auto* header = CMSG_FIRSTHDR(&message); header; header = CMSG_NXTHDR(&message, header)) {
                    if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS
                        && header->cmsg_len >= CMSG_LEN(sizeof(int))) {
                        std::memcpy(descriptor, CMSG_DATA(header), sizeof(int));
                        break;
                    }
                }
            }
            return true;
        }

        template<typename T>
        bool sendMessage(int socket, const T& value, int descriptor) {
            iovec io{const_cast<T*>(&value), sizeof(value)};
            alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int))]{};
            msghdr message{};
            message.msg_iov = &io;
            message.msg_iovlen = 1;
            if (descriptor >= 0) {
                message.msg_control = control;
                message.msg_controllen = sizeof(control);
                auto* header = CMSG_FIRSTHDR(&message);
                header->cmsg_level = SOL_SOCKET;
                header->cmsg_type = SCM_RIGHTS;
                header->cmsg_len = CMSG_LEN(sizeof(int));
                std::memcpy(CMSG_DATA(header), &descriptor, sizeof(int));
            }
            ssize_t sent;
            do { sent = ::sendmsg(socket, &message, MSG_NOSIGNAL); }
            while (sent < 0 && errno == EINTR);
            return sent == static_cast<ssize_t>(sizeof(value));
        }
    }

    bool sendRequest(int socket, const Request& request) { return sendMessage(socket, request, -1); }
    bool receiveRequest(int socket, Request& request) { return receiveMessage(socket, request, nullptr); }
    bool sendResponse(int socket, const Response& response, int descriptor) { return sendMessage(socket, response, descriptor); }
    bool receiveResponse(int socket, Response& response, int& descriptor) { return receiveMessage(socket, response, &descriptor); }
}