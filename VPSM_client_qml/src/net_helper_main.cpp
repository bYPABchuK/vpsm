#include "platform/linux/PrivilegedHelperProtocol.hpp"

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace vpsm::client::helper_protocol;

namespace {
int tunFd = -1;
std::string interfaceName;

Response responseFor(int status, const std::string& error = {}) {
    Response response;
    response.status = static_cast<std::int16_t>(status);
    std::strncpy(response.interfaceName, interfaceName.c_str(), InterfaceNameSize - 1);
    std::strncpy(response.error, error.c_str(), ErrorSize - 1);
    return response;
}

bool validName(const char* name) {
    const auto length = strnlen(name, InterfaceNameSize);
    if (length == 0 || length >= InterfaceNameSize) return false;
    const std::string value(name, length);
    if (value.rfind("vpsm", 0) != 0) return false;
    for (const unsigned char ch : value) {
        if (!(std::isalnum(ch) || ch == '_' || ch == '-')) return false;
    }
    return true;
}

std::string ipv4(std::uint32_t address) {
    return std::to_string((address >> 24) & 0xff) + "." +
           std::to_string((address >> 16) & 0xff) + "." +
           std::to_string((address >> 8) & 0xff) + "." +
           std::to_string(address & 0xff);
}

bool runIp(const std::vector<std::string>& arguments) {
    const char* executable = access("/usr/sbin/ip", X_OK) == 0 ? "/usr/sbin/ip" : "/usr/bin/ip";
    const auto child = fork();
    if (child < 0) return false;
    if (child == 0) {
        std::vector<char*> raw{const_cast<char*>(executable)};
        for (const auto& argument : arguments) raw.push_back(const_cast<char*>(argument.c_str()));
        raw.push_back(nullptr);
        execv(executable, raw.data());
        _exit(127);
    }
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void cleanup() {
    if (!interfaceName.empty()) {
        runIp({"route", "del", "10.240.0.0/16", "dev", interfaceName});
        runIp({"link", "set", "dev", interfaceName, "down"});
    }
    if (tunFd >= 0) close(tunFd);
    tunFd = -1;
    interfaceName.clear();
}

Response createTun(const Request& request) {
    if (tunFd >= 0) return responseFor(EALREADY, "TUN already exists");
    if (!validName(request.interfaceName)) return responseFor(EINVAL, "Invalid TUN interface name");
    const int descriptor = open("/dev/net/tun", O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (descriptor < 0) return responseFor(errno, std::strerror(errno));
    ifreq configuration{};
    configuration.ifr_flags = IFF_TUN | IFF_NO_PI;
    std::strncpy(configuration.ifr_name, request.interfaceName, IFNAMSIZ - 1);
    if (ioctl(descriptor, TUNSETIFF, &configuration) < 0) {
        const auto error = errno;
        close(descriptor);
        return responseFor(error, std::strerror(error));
    }
    tunFd = descriptor;
    interfaceName = configuration.ifr_name;
    return responseFor(0);
}

Response configure(const Request& request) {
    if (tunFd < 0) return responseFor(ENODEV, "TUN is not created");
    if (!validName(request.interfaceName) || interfaceName != request.interfaceName)
        return responseFor(EINVAL, "TUN interface mismatch");
    constexpr std::uint32_t network = 0x0AF00000u;
    if (request.networkAddress != network || request.prefixLength != 16
        || (request.localAddress & 0xFFFF0000u) != network
        || request.localAddress == network || request.localAddress == 0x0AF0FFFFu
        || request.mtu != 1400) return responseFor(EINVAL, "Rejected network configuration");
    const auto cidr = ipv4(request.localAddress) + "/16";
    if (!runIp({"link", "set", "dev", interfaceName, "mtu", "1400"})
        || !runIp({"addr", "replace", cidr, "dev", interfaceName})
        || !runIp({"link", "set", "dev", interfaceName, "up"})
        || !runIp({"route", "replace", "10.240.0.0/16", "dev", interfaceName})) {
        cleanup();
        return responseFor(EIO, "Failed to configure TUN with iproute2");
    }
    return responseFor(0);
}
}

int main() {
    if (geteuid() != 0) {
        std::fprintf(stderr, "vpsm_net_helper must run as root\n");
        return 77;
    }
    Request request;
    while (receiveRequest(STDIN_FILENO, request)) {
        if (request.magic != Magic || request.version != Version) {
            sendResponse(STDIN_FILENO, responseFor(EPROTO, "Invalid helper protocol"));
            continue;
        }
        if (request.command == Command::CreateTun) {
            const auto response = createTun(request);
            sendResponse(STDIN_FILENO, response, response.status == 0 ? tunFd : -1);
        } else if (request.command == Command::Configure) {
            sendResponse(STDIN_FILENO, configure(request));
        } else if (request.command == Command::Cleanup) {
            cleanup();
            sendResponse(STDIN_FILENO, responseFor(0));
        } else if (request.command == Command::Shutdown) {
            cleanup();
            sendResponse(STDIN_FILENO, responseFor(0));
            return 0;
        } else {
            sendResponse(STDIN_FILENO, responseFor(EINVAL, "Unknown helper command"));
        }
    }
    cleanup();
    return 0;
}