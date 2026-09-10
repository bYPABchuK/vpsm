#include "UdpTunnelTransport.hpp"

#include <QNetworkDatagram>

namespace vpsm::client {
    UdpTunnelTransport::UdpTunnelTransport(QObject* parent)
        : ITunnelTransport(parent), socket_(this) {}

    bool UdpTunnelTransport::open(const RouterEndpoint& endpoint, QString& error) {
        if (opened_) {
            error = QStringLiteral("UDP transport is already open");
            return false;
        }
        if (!endpoint.isValid()) {
            error = QStringLiteral("Invalid IPv4 router endpoint");
            return false;
        }
        if (!socket_.bind(QHostAddress::AnyIPv4, endpoint.localPort)) {
            error = socket_.errorString();
            return false;
        }
        endpoint_ = endpoint;
        opened_ = true;
        connect(&socket_, &QUdpSocket::readyRead,
                this, &UdpTunnelTransport::readPendingDatagrams,
                Qt::UniqueConnection);
        return true;
    }

    void UdpTunnelTransport::close() {
        if (!opened_) return;
        disconnect(&socket_, nullptr, this, nullptr);
        socket_.close();
        endpoint_ = {};
        opened_ = false;
    }

    bool UdpTunnelTransport::sendDatagram(const QByteArray& datagram, QString& error) {
        if (!opened_) {
            error = QStringLiteral("UDP transport is not open");
            return false;
        }
        const auto written = socket_.writeDatagram(datagram, endpoint_.address, endpoint_.port);
        if (written != datagram.size()) {
            error = socket_.errorString();
            return false;
        }
        return true;
    }

    void UdpTunnelTransport::readPendingDatagrams() {
        while (socket_.hasPendingDatagrams()) {
            const auto datagram = socket_.receiveDatagram();
            if (!datagram.isValid()) continue;
            if (datagram.senderAddress() != endpoint_.address ||
                datagram.senderPort() != endpoint_.port) continue;
            emit datagramReceived(datagram.data());
        }
    }
}