#include "LinuxTunDevice.hpp"
#include "LinuxPrivilegedHelperClient.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace vpsm::client {
    LinuxTunDevice::LinuxTunDevice(QObject* parent)
        : LinuxTunDevice(std::make_shared<LinuxPrivilegedHelperClient>(), parent) {}

    LinuxTunDevice::LinuxTunDevice(std::shared_ptr<LinuxPrivilegedHelperClient> helper, QObject* parent)
        : ITunDevice(parent), helper_(std::move(helper)) {}

    LinuxTunDevice::~LinuxTunDevice() {
        close();
    }

    bool LinuxTunDevice::create(const QString& preferredName, QString& error) {
        if (fd_ >= 0) {
            error = QStringLiteral("TUN device already exists");
            return false;
        }
        const auto requestedName = preferredName.toLocal8Bit();
        if (requestedName.isEmpty() || requestedName.size() >= 16) {
            error = QStringLiteral("Invalid TUN interface name");
            return false;
        }
        if (!helper_) {
            error = QStringLiteral("Privilege helper is not available");
            return false;
        }
        if (!helper_->createTun(preferredName, fd_, interfaceName_, error)) return false;
        const auto flags = ::fcntl(fd_, F_GETFL, 0);
        const auto descriptorFlags = ::fcntl(fd_, F_GETFD, 0);
        if (flags < 0 || descriptorFlags < 0
            || ::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0
            || ::fcntl(fd_, F_SETFD, descriptorFlags | FD_CLOEXEC) < 0) {
            error = QString::fromLocal8Bit(std::strerror(errno));
            ::close(fd_);
            fd_ = -1;
            helper_->cleanup();
            return false;
        }
        notifier_ = std::make_unique<QSocketNotifier>(fd_, QSocketNotifier::Read, this);
        connect(notifier_.get(), &QSocketNotifier::activated, this,
                [this](QSocketDescriptor, QSocketNotifier::Type) { readAvailablePackets(); });
        return true;
    }

    void LinuxTunDevice::close() {
        if (notifier_) {
            notifier_->setEnabled(false);
            notifier_.reset();
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        interfaceName_.clear();
    }

    bool LinuxTunDevice::writePacket(const QByteArray& packet, QString& error) {
        if (fd_ < 0) {
            error = QStringLiteral("TUN device is not open");
            return false;
        }
        const auto result = ::write(fd_, packet.constData(), static_cast<std::size_t>(packet.size()));
        if (result != packet.size()) {
            error = result < 0 ? QString::fromLocal8Bit(std::strerror(errno))
                               : QStringLiteral("Partial TUN packet write");
            return false;
        }
        return true;
    }

    void LinuxTunDevice::readAvailablePackets() {
        if (fd_ < 0) return;
        char buffer[2048];
        while (true) {
            const auto size = ::read(fd_, buffer, sizeof(buffer));
            if (size > 0) {
                emit packetReceived(QByteArray(buffer, static_cast<int>(size)));
                continue;
            }
            if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            if (size < 0 && errno == EINTR) continue;
            emit fatalError(size == 0 ? QStringLiteral("TUN device closed")
                                      : QString::fromLocal8Bit(std::strerror(errno)));
            return;
        }
    }
}