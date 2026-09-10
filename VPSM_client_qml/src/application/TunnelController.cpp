#include "TunnelController.hpp"

#include "protocol/Ipv4PacketParser.hpp"
#include "protocol/PacketCodecV2.hpp"

#include <limits>

namespace vpsm::client {
    TunnelController::TunnelController(
        ITunDevice& tunDevice,
        INetworkConfigurator& networkConfigurator,
        ITunnelTransport& transport,
        QObject* parent
    ) : QObject(parent),
        tunDevice_(tunDevice),
        networkConfigurator_(networkConfigurator),
        transport_(transport) {
        keepaliveTimer_.setInterval(20'000);
        registrationTimer_.setInterval(5'000);
        registrationTimer_.setSingleShot(true);

        connect(&tunDevice_, &ITunDevice::packetReceived,
                this, &TunnelController::handleTunPacket);
        connect(&tunDevice_, &ITunDevice::fatalError,
                this, &TunnelController::handleFatalError);
        connect(&transport_, &ITunnelTransport::datagramReceived,
                this, &TunnelController::handleDatagram);
        connect(&transport_, &ITunnelTransport::fatalError,
                this, &TunnelController::handleFatalError);
        connect(&keepaliveTimer_, &QTimer::timeout,
                this, &TunnelController::sendKeepalive);
        connect(&registrationTimer_, &QTimer::timeout,
                this, &TunnelController::handleRegistrationTimeout);
    }

    TunnelController::~TunnelController() {
        cleanup();
    }

    bool TunnelController::start(
        const Session& session,
        const VirtualNetwork& network,
        const RouterEndpoint& router,
        const QString& preferredInterfaceName
    ) {
        if (state_ != TunnelState::Disconnected && state_ != TunnelState::Error) return false;
        cleanup();
        errorText_.clear();
        emit errorChanged(errorText_);
        if (!session.isValid()) {
            fail(QStringLiteral("Invalid session credentials"));
            return false;
        }
        if (!network.isValid()) {
            fail(QStringLiteral("Invalid virtual network configuration"));
            return false;
        }
        if (!router.isValid()) {
            fail(QStringLiteral("Invalid router endpoint"));
            return false;
        }

        session_ = session;
        network_ = network;
        if (allowedNetworkIds_.isEmpty()) allowedNetworkIds_.insert(network.networkId);
        router_ = router;
        statistics_ = {};
        nextSequence_ = 1;
        replayWindow_.reset();
        emit statisticsChanged(statistics_);
        emit activeNetworkChanged();

        QString error;
        setState(TunnelState::PreparingInterface);
        if (!tunDevice_.create(preferredInterfaceName, error)) {
            fail(error.isEmpty() ? QStringLiteral("Failed to create TUN device") : error);
            return false;
        }
        interfaceCreated_ = true;
        interfaceConfiguration_ = InterfaceConfiguration{tunDevice_.interfaceName(), network_};
        networkConfigured_ = true;
        if (!networkConfigurator_.apply(interfaceConfiguration_, error)) {
            fail(error.isEmpty() ? QStringLiteral("Failed to configure TUN interface") : error);
            return false;
        }

        setState(TunnelState::BindingTransport);
        if (!transport_.open(router_, error)) {
            fail(error.isEmpty() ? QStringLiteral("Failed to bind UDP transport") : error);
            return false;
        }
        transportOpened_ = true;

        setState(TunnelState::RegisteringEndpoint);
        if (!sendPacket(
                PacketType::Keepalive,
                network_.localAddress.toIPv4Address(),
                network_.localAddress.toIPv4Address(),
                {})) {
            fail(QStringLiteral("Failed to send authenticated endpoint registration"));
            return false;
        }
        registrationTimer_.start();
        return true;
    }

    void TunnelController::stop() {
        if (state_ == TunnelState::Disconnected) return;
        setState(TunnelState::Disconnecting);
        cleanup();
        session_ = {};
        network_ = {};
        router_ = {};
        interfaceConfiguration_ = {};
        emit activeNetworkChanged();
        setState(TunnelState::Disconnected);
    }

    void TunnelController::setKeepaliveInterval(int milliseconds) {
        if (milliseconds > 0) keepaliveTimer_.setInterval(milliseconds);
    }

    void TunnelController::setRegistrationTimeout(int milliseconds) {
        if (milliseconds > 0) registrationTimer_.setInterval(milliseconds);
    }

    void TunnelController::setAllowedNetworkIds(const QSet<quint32>& networkIds) {
        allowedNetworkIds_ = networkIds;
        if (network_.networkId != 0) allowedNetworkIds_.insert(network_.networkId);
    }

    void TunnelController::setDestinationNetworks(const QHash<quint32, quint32>& networkByDestinationVip) {
        networkByDestinationVip_ = networkByDestinationVip;
    }

    void TunnelController::handleTunPacket(const QByteArray& packet) {
        if (state_ != TunnelState::Connected) {
            countDrop();
            return;
        }
        const auto parsed = Ipv4PacketParser::parse(packet);
        const auto local = network_.localAddress.toIPv4Address();
        const auto subnet = network_.networkAddress.toIPv4Address();
        if (!parsed || parsed->source != local ||
            !Ipv4PacketParser::belongsToSubnet(parsed->destination, subnet, network_.prefixLength)) {
            countDrop();
            return;
        }
        const auto exactPacket = packet.left(parsed->totalLength);
        const auto networkId = networkByDestinationVip_.value(parsed->destination, network_.networkId);
        if (!sendPacket(PacketType::Data, parsed->source, parsed->destination, exactPacket, networkId)) {
            fail(QStringLiteral("Failed to send TUN packet"));
        }
    }

    void TunnelController::handleDatagram(const QByteArray& datagram) {
        if (state_ != TunnelState::RegisteringEndpoint && state_ != TunnelState::Connected) return;
        const auto decoded = PacketCodecV2::decodeRouterPacket(session_, datagram);
        if (!decoded || !replayWindow_.accept(decoded->sequence)) {
            countDrop();
            return;
        }
        const auto local = network_.localAddress.toIPv4Address();
        if (!allowedNetworkIds_.contains(decoded->networkId) || decoded->destinationVip != local) {
            countDrop();
            return;
        }

        if (decoded->type == PacketType::Keepalive) {
            if (decoded->sourceVip != local || !decoded->payload.isEmpty()) {
                countDrop();
                return;
            }
            if (state_ == TunnelState::RegisteringEndpoint) {
                registrationTimer_.stop();
                keepaliveTimer_.start();
                setState(TunnelState::Connected);
            }
            return;
        }

        if (state_ != TunnelState::Connected || decoded->type != PacketType::Data) {
            countDrop();
            return;
        }
        const auto parsed = Ipv4PacketParser::parse(decoded->payload);
        if (!parsed || parsed->source != decoded->sourceVip ||
            parsed->destination != decoded->destinationVip ||
            !Ipv4PacketParser::belongsToSubnet(
                parsed->source,
                network_.networkAddress.toIPv4Address(),
                network_.prefixLength)) {
            countDrop();
            return;
        }
        QString error;
        const auto exactPacket = decoded->payload.left(parsed->totalLength);
        if (!tunDevice_.writePacket(exactPacket, error)) {
            fail(error.isEmpty() ? QStringLiteral("Failed to write packet to TUN") : error);
            return;
        }
        ++statistics_.rxPackets;
        statistics_.rxBytes += static_cast<quint64>(exactPacket.size());
        emit statisticsChanged(statistics_);
    }

    void TunnelController::handleFatalError(const QString& error) {
        fail(error.isEmpty() ? QStringLiteral("Tunnel backend failure") : error);
    }

    void TunnelController::sendKeepalive() {
        if (state_ != TunnelState::Connected) return;
        const auto local = network_.localAddress.toIPv4Address();
        if (!sendPacket(PacketType::Keepalive, local, local, {})) {
            fail(QStringLiteral("Failed to send keepalive"));
        }
    }

    void TunnelController::handleRegistrationTimeout() {
        if (state_ == TunnelState::RegisteringEndpoint) {
            fail(QStringLiteral("Router endpoint registration timed out"));
        }
    }

    bool TunnelController::sendPacket(
        PacketType type,
        quint32 sourceVip,
        quint32 destinationVip,
        const QByteArray& payload,
        quint32 networkId
    ) {
        if (nextSequence_ == 0 || nextSequence_ == std::numeric_limits<quint64>::max()) return false;
        const auto datagram = PacketCodecV2::encodeClientPacket(
            session_, nextSequence_++, type, networkId == 0 ? network_.networkId : networkId,
            sourceVip, destinationVip, payload
        );
        if (!datagram) return false;
        QString error;
        if (!transport_.sendDatagram(*datagram, error)) return false;
        if (type == PacketType::Data) {
            ++statistics_.txPackets;
            statistics_.txBytes += static_cast<quint64>(payload.size());
            emit statisticsChanged(statistics_);
        }
        return true;
    }

    void TunnelController::setState(TunnelState state) {
        if (state_ == state) return;
        state_ = state;
        emit stateChanged(state_);
    }

    void TunnelController::fail(const QString& error) {
        errorText_ = error;
        emit errorChanged(errorText_);
        cleanup();
        setState(TunnelState::Error);
    }

    void TunnelController::cleanup() {
        keepaliveTimer_.stop();
        registrationTimer_.stop();
        if (transportOpened_) {
            transport_.close();
            transportOpened_ = false;
        }
        if (networkConfigured_) {
            networkConfigurator_.remove(interfaceConfiguration_);
            networkConfigured_ = false;
        }
        if (interfaceCreated_) {
            tunDevice_.close();
            interfaceCreated_ = false;
        }
        replayWindow_.reset();
    }

    void TunnelController::countDrop() {
        ++statistics_.droppedPackets;
        emit statisticsChanged(statistics_);
    }
}