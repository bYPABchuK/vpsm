#include "StubPlatformServices.hpp"

#include "application/ports/INetworkConfigurator.hpp"
#include "application/ports/ITunDevice.hpp"

namespace vpsm::client {
    namespace {
        class UnsupportedTun final : public ITunDevice {
        public:
            using ITunDevice::ITunDevice;
            bool create(const QString&, QString& error) override {
                error = QStringLiteral("TUN backend is not implemented for this platform");
                return false;
            }
            void close() override {}
            bool writePacket(const QByteArray&, QString& error) override {
                error = QStringLiteral("TUN backend is not implemented for this platform");
                return false;
            }
            QString interfaceName() const override { return {}; }
        };

        class UnsupportedConfigurator final : public INetworkConfigurator {
        public:
            bool apply(const InterfaceConfiguration&, QString& error) override {
                error = QStringLiteral("Network configuration backend is not implemented for this platform");
                return false;
            }
            void remove(const InterfaceConfiguration&) override {}
        };

    }

    std::unique_ptr<ITunDevice> StubPlatformServices::createTunDevice() {
        return std::make_unique<UnsupportedTun>();
    }
    std::unique_ptr<INetworkConfigurator> StubPlatformServices::createNetworkConfigurator() {
        return std::make_unique<UnsupportedConfigurator>();
    }
}