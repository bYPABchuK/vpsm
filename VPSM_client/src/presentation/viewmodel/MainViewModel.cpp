#include "MainViewModel.hpp"

#include "ControlPlaneService.hpp"
#include "DataPlaneService.hpp"
#include "PacketRecord.hpp"

#include <QMetaObject>

namespace {
QString formatSec(qint64 sec)
{
    const qint64 h = sec / 3600;
    const qint64 m = (sec % 3600) / 60;
    const qint64 s = sec % 60;
    return QStringLiteral("%1-%2-%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}
}

MainViewModel::MainViewModel(DataPlaneService *dataPlane,
                             ControlPlaneService *controlPlane,
                             QObject *parent)
    : QObject(parent)
    , m_dataPlane(dataPlane)
    , m_controlPlane(controlPlane)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &MainViewModel::onTick);

    if (m_dataPlane) {
        connect(m_dataPlane, &DataPlaneService::packetSent, this, &MainViewModel::onPacketSent);
        connect(m_dataPlane, &DataPlaneService::packetReceived, this, &MainViewModel::onPacketReceived);
        connect(m_dataPlane, &DataPlaneService::errorOccurred, this, &MainViewModel::onDataPlaneError);
    }

    if (m_controlPlane) {
        connect(m_controlPlane, &ControlPlaneService::healthChecked, this, &MainViewModel::onHealthChecked);
    }
}

QString MainViewModel::timerDisplay() const { return m_timerDisplay; }
bool MainViewModel::connected() const { return m_connected; }
QString MainViewModel::statusText() const { return m_statusText; }

void MainViewModel::startSession(const QString &routerHost,
                                 quint16 routerPort,
                                 quint16 listenPort,
                                 int txThreads,
                                 int rxThreads,
                                 const QString &logPath)
{
    if (!m_dataPlane) {
        setStatus(QStringLiteral("Dataplane service is not available"));
        return;
    }

    const bool started = m_dataPlane->start(txThreads, rxThreads, listenPort, routerHost, routerPort);
    if (!started) {
        setStatus(QStringLiteral("Session already started"));
        return;
    }

    m_logWriter.start(logPath.toStdString());
    m_elapsedSec = 0;
    m_timerDisplay = formatSec(m_elapsedSec);
    emit timerDisplayChanged();

    m_connected = true;
    emit connectedChanged();

    setStatus(QStringLiteral("Connected"));
    m_timer.start();
}

void MainViewModel::stopSession()
{
    if (m_dataPlane) {
        m_dataPlane->stop();
    }
    m_logWriter.stop();
    m_timer.stop();
    m_elapsedSec = 0;
    m_timerDisplay = formatSec(0);
    emit timerDisplayChanged();

    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
    setStatus(QStringLiteral("Disconnected"));
}

void MainViewModel::sendPacket(const QString &srcVip,
                               const QString &destVip,
                               const QString &networkIp,
                               const QString &payload)
{
    if (!m_dataPlane) {
        return;
    }
    if (!m_dataPlane->enqueueSend(srcVip, destVip, networkIp, payload)) {
        setStatus(QStringLiteral("Cannot send packet: session is not running"));
    }
}

void MainViewModel::checkHealth(const QString &baseUrl)
{
    if (!m_controlPlane) {
        setStatus(QStringLiteral("ControlPlane service is not available"));
        return;
    }
    QMetaObject::invokeMethod(m_controlPlane, [this, baseUrl]() {
        m_controlPlane->checkHealth(baseUrl);
    }, Qt::QueuedConnection);
}

void MainViewModel::onTick()
{
    ++m_elapsedSec;
    m_timerDisplay = formatSec(m_elapsedSec);
    emit timerDisplayChanged();
}

void MainViewModel::onPacketSent(const PacketRecord &record)
{
    m_logWriter.enqueue(record);
    emit packetLineAdded(QStringLiteral("TX seq=%1 %2 -> %3 net=%4")
                             .arg(record.seq)
                             .arg(record.srcVip)
                             .arg(record.destVip)
                             .arg(record.networkIp));
}

void MainViewModel::onPacketReceived(const PacketRecord &record)
{
    m_logWriter.enqueue(record);
    emit packetLineAdded(QStringLiteral("RX seq=%1 %2 -> %3 net=%4 payload=%5")
                             .arg(record.seq)
                             .arg(record.srcVip)
                             .arg(record.destVip)
                             .arg(record.networkIp)
                             .arg(record.payload));
}

void MainViewModel::onDataPlaneError(const QString &message)
{
    setStatus(QStringLiteral("Dataplane error: %1").arg(message));
}

void MainViewModel::onHealthChecked(bool ok, const QString &message)
{
    setStatus(QStringLiteral("Health %1: %2").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")).arg(message));
}

void MainViewModel::setStatus(const QString &value)
{
    if (m_statusText == value) {
        return;
    }
    m_statusText = value;
    emit statusTextChanged();
}
