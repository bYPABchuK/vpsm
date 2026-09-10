#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QString>
#include <QtGlobal>

namespace vpsm::client {
    struct NetworkPeer {
        quint64 peerId = 0;
        QHostAddress address;
        QString nickname;
    };

    struct Session {
        quint64 peerId = 0;
        quint64 sessionId = 0;
        quint64 sessionKey = 0;
        QByteArray dataPlaneKey;

        bool isValid() const {
            return peerId != 0 && sessionId != 0 && sessionKey != 0 && dataPlaneKey.size() == 32;
        }
    };

    struct VirtualNetwork {
        quint32 networkId = 0;
        QString name;
        QHostAddress localAddress;
        QHostAddress networkAddress;
        int prefixLength = 0;
        int mtu = 1400;
        quint64 ownerPeerId = 0;
        QList<NetworkPeer> peers;

        bool isValid() const {
            if (networkId == 0 || localAddress.protocol() != QAbstractSocket::IPv4Protocol ||
                networkAddress.protocol() != QAbstractSocket::IPv4Protocol ||
                prefixLength <= 0 || prefixLength > 32 || mtu < 576 || mtu > 2002) return false;
            const quint32 mask = prefixLength == 32
                ? 0xffffffffu : (0xffffffffu << (32 - prefixLength));
            const quint32 local = localAddress.toIPv4Address();
            const quint32 network = networkAddress.toIPv4Address();
            if ((network & mask) != network || (local & mask) != network) return false;
            if (prefixLength <= 30) {
                const quint32 broadcast = network | ~mask;
                if (local == network || local == broadcast) return false;
            }
            return true;
        }
    };

    struct RouterEndpoint {
        QHostAddress address;
        quint16 port = 0;
        quint16 localPort = 0;

        bool isValid() const {
            return address.protocol() == QAbstractSocket::IPv4Protocol && port != 0 && localPort != 0;
        }
    };

    struct TunnelStatistics {
        quint64 txPackets = 0;
        quint64 rxPackets = 0;
        quint64 txBytes = 0;
        quint64 rxBytes = 0;
        quint64 droppedPackets = 0;
    };

    enum class TunnelState {
        Disconnected,
        PreparingInterface,
        BindingTransport,
        RegisteringEndpoint,
        Connected,
        Disconnecting,
        Error,
    };
}

Q_DECLARE_METATYPE(vpsm::client::TunnelState)
Q_DECLARE_METATYPE(vpsm::client::TunnelStatistics)
Q_DECLARE_METATYPE(vpsm::client::Session)
Q_DECLARE_METATYPE(vpsm::client::VirtualNetwork)