#include "LinuxPrivilegedHelperClient.hpp"
#include "PrivilegedHelperProtocol.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <spawn.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

extern char** environ;

namespace vpsm::client {
    using namespace helper_protocol;

    namespace {
        QString helperPath() {
            const auto besideApplication = QDir(QCoreApplication::applicationDirPath())
                .filePath(QStringLiteral("vpsm_net_helper"));
            if (QFileInfo(besideApplication).isExecutable()) return besideApplication;
            const QString installed = QStringLiteral("/usr/libexec/vpsm/vpsm_net_helper");
            return QFileInfo(installed).isExecutable() ? installed : QString{};
        }

        QString responseError(const Response& response) {
            return QString::fromLocal8Bit(response.error, strnlen(response.error, ErrorSize));
        }

        QString preferredPrivilegeTool() {
            const auto doas = QStandardPaths::findExecutable(QStringLiteral("doas"));
            if (!doas.isEmpty()) return doas;
            return QStandardPaths::findExecutable(QStringLiteral("sudo"));
        }
    }

    LinuxPrivilegedHelperClient::~LinuxPrivilegedHelperClient() { shutdown(); }

    bool LinuxPrivilegedHelperClient::ensureStarted(QString& error) {
        if (socket_ >= 0) return true;
        const auto elevationTool = requestedElevationTool_.isEmpty()
            ? preferredPrivilegeTool() : requestedElevationTool_;
        requestedElevationTool_.clear();
        return startHelper(elevationTool, error);
    }

    bool LinuxPrivilegedHelperClient::startHelper(const QString& elevationTool, QString& error) {
        const auto helper = helperPath();
        if (helper.isEmpty()) {
            error = QStringLiteral("Cannot find vpsm_net_helper beside the client or in /usr/libexec/vpsm");
            return false;
        }
        if (::geteuid() != 0 && elevationTool.isEmpty()) {
            error = QStringLiteral("Cannot find doas or sudo. Install one of them to start the privileged network helper");
            return false;
        }

        if (::fcntl(STDIN_FILENO, F_GETFD) < 0) {
            const int nullInput = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            if (nullInput < 0 || (nullInput != STDIN_FILENO && ::dup2(nullInput, STDIN_FILENO) < 0)) {
                if (nullInput >= 0) ::close(nullInput);
                error = QStringLiteral("Cannot reserve the helper control descriptor");
                return false;
            }
            if (nullInput != STDIN_FILENO) ::close(nullInput);
        }

        int sockets[2] = {-1, -1};
        if (::socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) < 0) {
            error = QString::fromLocal8Bit(std::strerror(errno));
            return false;
        }
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, sockets[1], STDIN_FILENO);
        posix_spawn_file_actions_addclose(&actions, sockets[0]);
        posix_spawn_file_actions_addclose(&actions, sockets[1]);

        const auto elevationToolBytes = elevationTool.toLocal8Bit();
        const auto helperBytes = helper.toLocal8Bit();
        char* elevatedArguments[] = {const_cast<char*>(elevationToolBytes.constData()), const_cast<char*>(helperBytes.constData()), nullptr};
        char* rootArguments[] = {const_cast<char*>(helperBytes.constData()), nullptr};
        const bool alreadyRoot = ::geteuid() == 0;
        pid_t child = -1;
        const int result = posix_spawnp(&child,
            alreadyRoot ? helperBytes.constData() : elevationToolBytes.constData(), &actions, nullptr,
            alreadyRoot ? rootArguments : elevatedArguments, environ);
        posix_spawn_file_actions_destroy(&actions);
        ::close(sockets[1]);
        if (result != 0) {
            ::close(sockets[0]);
            error = QStringLiteral("Cannot start privilege tool: %1").arg(QString::fromLocal8Bit(std::strerror(result)));
            return false;
        }
        socket_ = sockets[0];
        childPid_ = static_cast<int>(child);
        activeElevationTool_ = alreadyRoot ? QString{} : elevationTool;
        return true;
    }

    bool LinuxPrivilegedHelperClient::transact(
        const void* requestPointer, void* responsePointer, int& descriptor, QString& error
    ) {
        if (!ensureStarted(error)) return false;
        const auto& request = *static_cast<const Request*>(requestPointer);
        auto& response = *static_cast<Response*>(responsePointer);
        descriptor = -1;
        const auto exchange = [&]() {
            return sendRequest(socket_, request) && receiveResponse(socket_, response, descriptor);
        };
        if (!exchange()) {
            if (descriptor >= 0) ::close(descriptor);
            descriptor = -1;
            const bool failedThroughDoas = QFileInfo(activeElevationTool_).fileName() == QStringLiteral("doas");
            shutdown();
            const auto sudo = QStandardPaths::findExecutable(QStringLiteral("sudo"));
            if (failedThroughDoas && !sudo.isEmpty()) {
                requestedElevationTool_ = sudo;
                if (ensureStarted(error) && exchange()) {
                    // Continue with normal protocol/status validation below.
                } else {
                    if (descriptor >= 0) ::close(descriptor);
                    descriptor = -1;
                    shutdown();
                    error = QStringLiteral("Both doas and sudo failed to start the privileged helper. Start VPSM from a terminal and verify sudo permissions");
                    return false;
                }
            } else {
                error = QStringLiteral("Privilege helper terminated or authorization was cancelled. Start VPSM from a terminal if doas or sudo could not request a password");
                return false;
            }
        }
        if (response.magic != Magic || response.version != Version || response.status != 0) {
            if (descriptor >= 0) ::close(descriptor);
            error = responseError(response);
            if (error.isEmpty()) error = QStringLiteral("Privilege helper failed (%1)").arg(response.status);
            return false;
        }
        return true;
    }

    bool LinuxPrivilegedHelperClient::createTun(
        const QString& preferredName, int& descriptor, QString& actualName, QString& error
    ) {
        Request request;
        request.command = Command::CreateTun;
        const auto name = preferredName.toLocal8Bit();
        if (name.isEmpty() || name.size() >= static_cast<int>(InterfaceNameSize)) {
            error = QStringLiteral("Invalid TUN interface name");
            return false;
        }
        std::strncpy(request.interfaceName, name.constData(), InterfaceNameSize - 1);
        Response response;
        if (!transact(&request, &response, descriptor, error)) return false;
        if (descriptor < 0) {
            error = QStringLiteral("Privilege helper did not return a TUN descriptor");
            return false;
        }
        actualName = QString::fromLocal8Bit(response.interfaceName, strnlen(response.interfaceName, InterfaceNameSize));
        return true;
    }

    bool LinuxPrivilegedHelperClient::configure(const InterfaceConfiguration& configuration, QString& error) {
        Request request;
        request.command = Command::Configure;
        request.localAddress = configuration.network.localAddress.toIPv4Address();
        request.networkAddress = configuration.network.networkAddress.toIPv4Address();
        request.prefixLength = static_cast<std::uint8_t>(configuration.network.prefixLength);
        request.mtu = static_cast<std::uint16_t>(configuration.network.mtu);
        const auto name = configuration.interfaceName.toLocal8Bit();
        std::strncpy(request.interfaceName, name.constData(), InterfaceNameSize - 1);
        Response response;
        int descriptor = -1;
        return transact(&request, &response, descriptor, error);
    }

    void LinuxPrivilegedHelperClient::cleanup() {
        if (socket_ < 0) return;
        Request request;
        request.command = Command::Cleanup;
        Response response;
        int descriptor = -1;
        QString ignored;
        transact(&request, &response, descriptor, ignored);
    }

    void LinuxPrivilegedHelperClient::shutdown() {
        if (socket_ >= 0) {
            Request request;
            request.command = Command::Shutdown;
            sendRequest(socket_, request);
            Response response;
            int descriptor = -1;
            receiveResponse(socket_, response, descriptor);
            if (descriptor >= 0) ::close(descriptor);
            ::close(socket_);
            socket_ = -1;
        }
        if (childPid_ > 0) {
            int status = 0;
            while (::waitpid(childPid_, &status, 0) < 0 && errno == EINTR) {}
            childPid_ = -1;
        }
        activeElevationTool_.clear();
    }
}