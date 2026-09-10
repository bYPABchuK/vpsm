#pragma once

#include "domain/TunnelTypes.hpp"

#include <QByteArray>
#include <QObject>
#include <QString>

namespace vpsm::client {
    class ITunnelTransport : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;
        ~ITunnelTransport() override = default;

        virtual bool open(const RouterEndpoint& endpoint, QString& error) = 0;
        virtual void close() = 0;
        virtual bool sendDatagram(const QByteArray& datagram, QString& error) = 0;

    signals:
        void datagramReceived(const QByteArray& datagram);
        void fatalError(const QString& message);
    };
}