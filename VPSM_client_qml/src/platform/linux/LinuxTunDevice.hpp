#pragma once

#include "application/ports/ITunDevice.hpp"

#include <QSocketNotifier>
#include <memory>

namespace vpsm::client {
    class LinuxPrivilegedHelperClient;

    class LinuxTunDevice final : public ITunDevice {
        Q_OBJECT
    public:
        explicit LinuxTunDevice(QObject* parent = nullptr);
        explicit LinuxTunDevice(std::shared_ptr<LinuxPrivilegedHelperClient> helper, QObject* parent = nullptr);
        ~LinuxTunDevice() override;

        bool create(const QString& preferredName, QString& error) override;
        void close() override;
        bool writePacket(const QByteArray& packet, QString& error) override;
        QString interfaceName() const override { return interfaceName_; }

    private:
        void readAvailablePackets();

        int fd_ = -1;
        std::shared_ptr<LinuxPrivilegedHelperClient> helper_;
        QString interfaceName_;
        std::unique_ptr<QSocketNotifier> notifier_;
    };
}