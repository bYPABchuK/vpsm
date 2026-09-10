#pragma once

#include "application/ports/INetworkConfigurator.hpp"
#include "application/ports/ITunDevice.hpp"
#include "application/ports/ITunnelTransport.hpp"
#include "application/ports/IControlPlaneClient.hpp"

#include <QtEndian>

#include <algorithm>

namespace vpsm::client::test {
    class FakeControlPlaneClient final : public IControlPlaneClient {
    public:
        using IControlPlaneClient::IControlPlaneClient;
        bool busyValue = false;
        QString lastServer;
        QString lastNickname;
        QString lastPassword;
        quint32 lastNetworkId = 0;
        int probeCalls = 0;
        int loginCalls = 0;
        int joinCalls = 0;
        int listCalls = 0;
        int peerListCalls = 0;
        int createCalls = 0;
        int leaveCalls = 0;
        int deleteCalls = 0;
        int logoutCalls = 0;
        quint64 lastPeerId = 0;

        void probeServer(const QString& server) override {
            ++probeCalls;
            lastServer = server;
        }
        void login(const QString& server, const QString& nickname, const QString& password) override {
            ++loginCalls;
            lastServer = server;
            lastNickname = nickname;
            lastPassword = password;
        }
        void joinNetwork(quint32 networkId, const QString& password) override {
            ++joinCalls;
            lastNetworkId = networkId;
            lastPassword = password;
        }
        void joinNetworkByName(const QString& name, const QString& password) override {
            ++joinCalls;
            lastNickname = name;
            lastPassword = password;
        }
        void listNetworks() override { ++listCalls; }
        void listNetworkPeers() override { ++peerListCalls; }
        void createNetwork(const QString& name, const QString& password) override {
            ++createCalls;
            lastNickname = name;
            lastPassword = password;
        }
        void leaveNetwork(quint32 networkId, quint64 peerId) override {
            ++leaveCalls;
            lastNetworkId = networkId;
            lastPeerId = peerId;
        }
        void deleteNetwork(quint32 networkId) override {
            ++deleteCalls;
            lastNetworkId = networkId;
        }
        void logout() override { ++logoutCalls; }
        bool busy() const override { return busyValue; }
        void completeServerProbe(const QString& error = {}) {
            emit serverProbeCompleted(error);
        }
        void completeLogin(const Session& session, const QString& error = {}) {
            emit loginCompleted(session, error);
        }
        void completeJoin(const VirtualNetwork& network, const QString& error = {}) {
            emit networkJoined(network, error);
        }
        void completeList(const QList<VirtualNetwork>& networks, const QString& error = {}) {
            emit networksListed(networks, error);
        }
        void completePeerList(const QList<VirtualNetwork>& networks, const QString& error = {}) {
            emit networkPeersListed(networks, error);
        }
        void completeCreate(quint32 networkId, const QString& error = {}) {
            emit networkCreated(networkId, error);
        }
        void completeMutation(const QString& error = {}) { emit networkMutationCompleted(error); }
        void completeLogout() { emit loggedOut(); }
    };

    class FakeTunDevice final : public ITunDevice {
    public:
        using ITunDevice::ITunDevice;
        bool createResult = true;
        bool writeResult = true;
        bool created = false;
        bool closed = false;
        QString name = QStringLiteral("vpsm-test0");
        QList<QByteArray> writtenPackets;

        bool create(const QString&, QString& error) override {
            if (!createResult) {
                error = QStringLiteral("fake TUN create failure");
                return false;
            }
            created = true;
            closed = false;
            return true;
        }
        void close() override { closed = true; created = false; }
        bool writePacket(const QByteArray& packet, QString& error) override {
            if (!writeResult) {
                error = QStringLiteral("fake TUN write failure");
                return false;
            }
            writtenPackets.push_back(packet);
            return true;
        }
        QString interfaceName() const override { return name; }
        void inject(const QByteArray& packet) { emit packetReceived(packet); }
        void injectError(const QString& error) { emit fatalError(error); }
    };

    class FakeNetworkConfigurator final : public INetworkConfigurator {
    public:
        bool applyResult = true;
        int applyCalls = 0;
        int removeCalls = 0;
        InterfaceConfiguration lastConfiguration;

        bool apply(const InterfaceConfiguration& configuration, QString& error) override {
            ++applyCalls;
            lastConfiguration = configuration;
            if (!applyResult) {
                error = QStringLiteral("fake network configuration failure");
                return false;
            }
            return true;
        }
        void remove(const InterfaceConfiguration&) override { ++removeCalls; }
    };

    class FakeTunnelTransport final : public ITunnelTransport {
    public:
        using ITunnelTransport::ITunnelTransport;
        bool openResult = true;
        bool sendResult = true;
        bool opened = false;
        int closeCalls = 0;
        RouterEndpoint endpoint;
        QList<QByteArray> sentDatagrams;

        bool open(const RouterEndpoint& value, QString& error) override {
            if (!openResult) {
                error = QStringLiteral("fake transport open failure");
                return false;
            }
            endpoint = value;
            opened = true;
            return true;
        }
        void close() override { opened = false; ++closeCalls; }
        bool sendDatagram(const QByteArray& datagram, QString& error) override {
            if (!sendResult) {
                error = QStringLiteral("fake transport send failure");
                return false;
            }
            sentDatagrams.push_back(datagram);
            return true;
        }
        void inject(const QByteArray& datagram) { emit datagramReceived(datagram); }
        void injectError(const QString& error) { emit fatalError(error); }
    };

    inline Session validSession() {
        return Session{1, 1001, 2001, QByteArray(32, '\x42')};
    }

    inline VirtualNetwork validNetwork() {
        return VirtualNetwork{
            10,
            QStringLiteral("test-network"),
            QHostAddress(QStringLiteral("10.240.1.1")),
            QHostAddress(QStringLiteral("10.240.1.0")),
            24,
            1400,
        };
    }

    inline RouterEndpoint validRouter() {
        return RouterEndpoint{QHostAddress(QStringLiteral("127.0.0.1")), 4000, 4001};
    }

    inline QByteArray ipv4Packet(quint32 source, quint32 destination, QByteArray payload = {}) {
        const int total = 20 + payload.size();
        QByteArray packet(total, '\0');
        auto* bytes = reinterpret_cast<uchar*>(packet.data());
        bytes[0] = 0x45;
        qToBigEndian<quint16>(static_cast<quint16>(total), bytes + 2);
        bytes[8] = 64;
        bytes[9] = 6;
        qToBigEndian<quint32>(source, bytes + 12);
        qToBigEndian<quint32>(destination, bytes + 16);
        if (!payload.isEmpty()) std::copy(payload.cbegin(), payload.cend(), packet.begin() + 20);
        return packet;
    }
}