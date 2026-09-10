#pragma once

#include "application/ports/INetworkConfigurator.hpp"
#include "application/ports/ITunDevice.hpp"
#include "application/ports/ITunnelTransport.hpp"
#include "domain/TunnelTypes.hpp"
#include "protocol/PacketCodecV2.hpp"
#include "protocol/ReplayWindow.hpp"

#include <QObject>
#include <QHash>
#include <QSet>
#include <QTimer>

namespace vpsm::client {
    class TunnelController final : public QObject {
        Q_OBJECT
    public:
        TunnelController(
            ITunDevice& tunDevice,
            INetworkConfigurator& networkConfigurator,
            ITunnelTransport& transport,
            QObject* parent = nullptr
        );
        ~TunnelController() override;

        TunnelState state() const { return state_; }
        QString errorText() const { return errorText_; }
        TunnelStatistics statistics() const { return statistics_; }
        VirtualNetwork activeNetwork() const { return network_; }
        QString interfaceName() const { return interfaceConfiguration_.interfaceName; }

        bool start(
            const Session& session,
            const VirtualNetwork& network,
            const RouterEndpoint& router,
            const QString& preferredInterfaceName = QStringLiteral("vpsm0")
        );
        void stop();

        void setKeepaliveInterval(int milliseconds);
        void setRegistrationTimeout(int milliseconds);
        void setAllowedNetworkIds(const QSet<quint32>& networkIds);
        void setDestinationNetworks(const QHash<quint32, quint32>& networkByDestinationVip);

    signals:
        void stateChanged(vpsm::client::TunnelState state);
        void errorChanged(const QString& error);
        void statisticsChanged(const vpsm::client::TunnelStatistics& statistics);
        void activeNetworkChanged();

    private slots:
        void handleTunPacket(const QByteArray& packet);
        void handleDatagram(const QByteArray& datagram);
        void handleFatalError(const QString& error);
        void sendKeepalive();
        void handleRegistrationTimeout();

    private:
        bool sendPacket(
            PacketType type,
            quint32 sourceVip,
            quint32 destinationVip,
            const QByteArray& payload,
            quint32 networkId = 0
        );
        void setState(TunnelState state);
        void fail(const QString& error);
        void cleanup();
        void countDrop();

        ITunDevice& tunDevice_;
        INetworkConfigurator& networkConfigurator_;
        ITunnelTransport& transport_;
        Session session_;
        VirtualNetwork network_;
        QSet<quint32> allowedNetworkIds_;
        QHash<quint32, quint32> networkByDestinationVip_;
        RouterEndpoint router_;
        InterfaceConfiguration interfaceConfiguration_;
        ReplayWindow replayWindow_;
        QTimer keepaliveTimer_;
        QTimer registrationTimer_;
        TunnelState state_ = TunnelState::Disconnected;
        TunnelStatistics statistics_;
        QString errorText_;
        quint64 nextSequence_ = 1;
        bool interfaceCreated_ = false;
        bool networkConfigured_ = false;
        bool transportOpened_ = false;
    };
}