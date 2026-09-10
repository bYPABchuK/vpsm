#pragma once

#include "application/TunnelController.hpp"

#include <QObject>

namespace vpsm::client {
    class TunnelViewModel final : public QObject {
        Q_OBJECT
        Q_PROPERTY(int state READ state NOTIFY stateChanged)
        Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
        Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
        Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
        Q_PROPERTY(QString interfaceName READ interfaceName NOTIFY networkChanged)
        Q_PROPERTY(QString localAddress READ localAddress NOTIFY networkChanged)
        Q_PROPERTY(qulonglong txPackets READ txPackets NOTIFY statisticsChanged)
        Q_PROPERTY(qulonglong rxPackets READ rxPackets NOTIFY statisticsChanged)
        Q_PROPERTY(qulonglong txBytes READ txBytes NOTIFY statisticsChanged)
        Q_PROPERTY(qulonglong rxBytes READ rxBytes NOTIFY statisticsChanged)
        Q_PROPERTY(qulonglong droppedPackets READ droppedPackets NOTIFY statisticsChanged)

    public:
        explicit TunnelViewModel(TunnelController& controller, QObject* parent = nullptr);

        int state() const;
        QString stateText() const;
        bool connected() const;
        bool busy() const;
        QString errorText() const;
        QString interfaceName() const;
        QString localAddress() const;
        qulonglong txPackets() const;
        qulonglong rxPackets() const;
        qulonglong txBytes() const;
        qulonglong rxBytes() const;
        qulonglong droppedPackets() const;

        bool connectTunnel(
            const Session& session,
            const VirtualNetwork& network,
            const RouterEndpoint& router,
            const QString& interfaceName = QStringLiteral("vpsm0")
        );
        void setAllowedNetworkIds(const QSet<quint32>& networkIds);
        void setDestinationNetworks(const QHash<quint32, quint32>& networkByDestinationVip);
        Q_INVOKABLE void disconnectTunnel();

    signals:
        void stateChanged();
        void errorTextChanged();
        void networkChanged();
        void statisticsChanged();

    private:
        TunnelController& controller_;
    };
}