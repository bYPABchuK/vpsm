#pragma once

#include "application/ports/ITunnelTransport.hpp"

#include <QUdpSocket>

namespace vpsm::client {
    class UdpTunnelTransport final : public ITunnelTransport {
        Q_OBJECT
    public:
        explicit UdpTunnelTransport(QObject* parent = nullptr);

        bool open(const RouterEndpoint& endpoint, QString& error) override;
        void close() override;
        bool sendDatagram(const QByteArray& datagram, QString& error) override;

    private slots:
        void readPendingDatagrams();

    private:
        QUdpSocket socket_;
        RouterEndpoint endpoint_;
        bool opened_ = false;
    };
}