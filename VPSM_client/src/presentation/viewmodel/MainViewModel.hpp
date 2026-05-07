#pragma once

#include "PacketLogWriter.hpp"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtGlobal>

class DataPlaneService;
class ControlPlaneService;
struct PacketRecord;

class MainViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString timerDisplay READ timerDisplay NOTIFY timerDisplayChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit MainViewModel(DataPlaneService *dataPlane,
                           ControlPlaneService *controlPlane,
                           QObject *parent = nullptr);

    QString timerDisplay() const;
    bool connected() const;
    QString statusText() const;

public slots:
    void startSession(const QString &routerHost,
                      quint16 routerPort,
                      quint16 listenPort,
                      int txThreads,
                      int rxThreads,
                      const QString &logPath);
    void stopSession();
    void sendPacket(const QString &srcVip,
                    const QString &destVip,
                    const QString &networkIp,
                    const QString &payload);
    void checkHealth(const QString &baseUrl);

signals:
    void timerDisplayChanged();
    void connectedChanged();
    void statusTextChanged();
    void packetLineAdded(const QString &line);

private slots:
    void onTick();
    void onPacketSent(const PacketRecord &record);
    void onPacketReceived(const PacketRecord &record);
    void onDataPlaneError(const QString &message);
    void onHealthChecked(bool ok, const QString &message);

private:
    void setStatus(const QString &value);

    DataPlaneService *m_dataPlane {nullptr};
    ControlPlaneService *m_controlPlane {nullptr};
    PacketLogWriter m_logWriter;
    QTimer m_timer;
    qint64 m_elapsedSec {0};
    QString m_timerDisplay {QStringLiteral("00-00-00")};
    QString m_statusText {QStringLiteral("Idle")};
    bool m_connected {false};
};
