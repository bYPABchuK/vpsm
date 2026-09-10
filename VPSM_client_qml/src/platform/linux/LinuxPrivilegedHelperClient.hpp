#pragma once

#include "application/ports/INetworkConfigurator.hpp"

#include <QString>

namespace vpsm::client {
    class LinuxPrivilegedHelperClient final {
    public:
        LinuxPrivilegedHelperClient() = default;
        ~LinuxPrivilegedHelperClient();
        LinuxPrivilegedHelperClient(const LinuxPrivilegedHelperClient&) = delete;
        LinuxPrivilegedHelperClient& operator=(const LinuxPrivilegedHelperClient&) = delete;

        bool createTun(const QString& preferredName, int& descriptor, QString& actualName, QString& error);
        bool configure(const InterfaceConfiguration& configuration, QString& error);
        void cleanup();

    private:
        bool ensureStarted(QString& error);
        bool startHelper(const QString& elevationTool, QString& error);
        bool transact(const void* request, void* response, int& descriptor, QString& error);
        void shutdown();

        int socket_ = -1;
        int childPid_ = -1;
        QString activeElevationTool_;
        QString requestedElevationTool_;
    };
}