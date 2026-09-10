#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace vpsm::client {
    class ITunDevice : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;
        ~ITunDevice() override = default;

        virtual bool create(const QString& preferredName, QString& error) = 0;
        virtual void close() = 0;
        virtual bool writePacket(const QByteArray& packet, QString& error) = 0;
        virtual QString interfaceName() const = 0;

    signals:
        void packetReceived(const QByteArray& packet);
        void fatalError(const QString& message);
    };
}