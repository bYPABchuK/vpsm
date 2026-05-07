#include "DataPlaneService.hpp"

#include <QDateTime>
#include <QHostAddress>
#include <QMetaObject>
#include <QtEndian>

#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

DataPlaneService::DataPlaneService(QObject *parent)
    : QObject(parent)
{
}

DataPlaneService::~DataPlaneService()
{
    stop();
}

bool DataPlaneService::start(int txThreads,
                             int rxThreads,
                             quint16 listenPort,
                             const QString &routerHost,
                             quint16 routerPort)
{
    if (m_running.exchange(true)) {
        return false;
    }

    m_listenPort = listenPort;
    m_routerHost = routerHost;
    m_routerPort = routerPort;

    txThreads = std::max(1, txThreads);
    rxThreads = std::max(1, rxThreads);

    m_txWorkers.reserve(static_cast<size_t>(txThreads));
    m_rxWorkers.reserve(static_cast<size_t>(rxThreads));

    for (int i = 0; i < txThreads; ++i) {
        m_txWorkers.emplace_back(&DataPlaneService::txLoop, this);
    }
    for (int i = 0; i < rxThreads; ++i) {
        m_rxWorkers.emplace_back(&DataPlaneService::rxLoop, this);
    }

    return true;
}

void DataPlaneService::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    m_txCv.notify_all();

    for (auto &w : m_txWorkers) {
        if (w.joinable()) {
            w.join();
        }
    }
    m_txWorkers.clear();

    for (auto &w : m_rxWorkers) {
        if (w.joinable()) {
            w.join();
        }
    }
    m_rxWorkers.clear();
}

bool DataPlaneService::enqueueSend(const QString &srcVip,
                                   const QString &destVip,
                                   const QString &networkIp,
                                   const QString &payload)
{
    if (!m_running.load()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_txMutex);
        m_txQueue.push_back(SendTask {srcVip, destVip, networkIp, payload});
    }
    m_txCv.notify_one();
    return true;
}

void DataPlaneService::txLoop()
{
    const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("TX socket create failed")); }, Qt::QueuedConnection);
        return;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_routerPort);
    if (::inet_pton(AF_INET, m_routerHost.toUtf8().constData(), &addr.sin_addr) != 1) {
        QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("Invalid router host")); }, Qt::QueuedConnection);
        ::close(sock);
        return;
    }

    while (m_running.load()) {
        SendTask task;
        {
            std::unique_lock<std::mutex> lock(m_txMutex);
            m_txCv.wait_for(lock, std::chrono::milliseconds(200), [this] {
                return !m_running.load() || !m_txQueue.empty();
            });
            if (!m_running.load()) {
                break;
            }
            if (m_txQueue.empty()) {
                continue;
            }
            task = m_txQueue.front();
            m_txQueue.pop_front();
        }

        quint32 src = 0;
        quint32 dst = 0;
        quint32 net = 0;
        if (!parseIpv4(task.srcVip, src) || !parseIpv4(task.destVip, dst) || !parseIpv4(task.networkIp, net)) {
            QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("Invalid src/dst/network IPv4")); }, Qt::QueuedConnection);
            continue;
        }

        const quint64 seq = m_nextSeq.fetch_add(1);
        const QByteArray packet = buildPacket(seq, m_sessionId, net, src, dst, task.payload.toUtf8());
        const ssize_t sent = ::sendto(sock,
                                      packet.constData(),
                                      static_cast<size_t>(packet.size()),
                                      0,
                                      reinterpret_cast<const sockaddr *>(&addr),
                                      sizeof(addr));
        if (sent < 0) {
            QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("sendto failed")); }, Qt::QueuedConnection);
            continue;
        }

        PacketRecord rec;
        rec.srcVip = task.srcVip;
        rec.destVip = task.destVip;
        rec.networkIp = task.networkIp;
        rec.payload = task.payload;
        rec.seq = seq;
        rec.timestampIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        QMetaObject::invokeMethod(this, [this, rec]() { emit packetSent(rec); }, Qt::QueuedConnection);
    }

    ::close(sock);
}

void DataPlaneService::rxLoop()
{
    const int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("RX socket create failed")); }, Qt::QueuedConnection);
        return;
    }

    int one = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_REUSEPORT
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
#endif

    timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in bindAddr {};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindAddr.sin_port = htons(m_listenPort);

    if (::bind(sock, reinterpret_cast<const sockaddr *>(&bindAddr), sizeof(bindAddr)) < 0) {
        QMetaObject::invokeMethod(this, [this]() { emit errorOccurred(QStringLiteral("bind failed for RX socket")); }, Qt::QueuedConnection);
        ::close(sock);
        return;
    }

    char buf[65536];
    while (m_running.load()) {
        sockaddr_in from {};
        socklen_t fromLen = sizeof(from);
        const ssize_t n = ::recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<sockaddr *>(&from), &fromLen);
        if (n <= 0) {
            continue;
        }
        if (n < 30) {
            continue;
        }

        const auto *p = reinterpret_cast<const uchar *>(buf);
        const quint8 version = p[0];
        if (version != 2) {
            continue;
        }

        const quint64 session = qFromBigEndian<quint64>(p + 1);
        Q_UNUSED(session)
        const quint64 seq = qFromBigEndian<quint64>(p + 9);
        Q_UNUSED(p[17])
        const quint32 network = qFromBigEndian<quint32>(p + 18);
        const quint32 src = qFromBigEndian<quint32>(p + 22);
        const quint32 dst = qFromBigEndian<quint32>(p + 26);

        PacketRecord rec;
        rec.seq = seq;
        rec.networkIp = ipv4ToString(network);
        rec.srcVip = ipv4ToString(src);
        rec.destVip = ipv4ToString(dst);
        rec.payload = QString::fromUtf8(buf + 30, static_cast<int>(n - 30));
        rec.timestampIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        QMetaObject::invokeMethod(this, [this, rec]() { emit packetReceived(rec); }, Qt::QueuedConnection);
    }

    ::close(sock);
}

bool DataPlaneService::parseIpv4(const QString &value, quint32 &out)
{
    QHostAddress addr;
    if (!addr.setAddress(value)) {
        return false;
    }
    out = addr.toIPv4Address();
    return true;
}

QString DataPlaneService::ipv4ToString(quint32 value)
{
    return QHostAddress(value).toString();
}

QByteArray DataPlaneService::buildPacket(quint64 seq,
                                         quint64 sessionId,
                                         quint32 networkId,
                                         quint32 srcVip,
                                         quint32 dstVip,
                                         const QByteArray &payload)
{
    QByteArray out;
    out.resize(30 + payload.size());
    uchar *p = reinterpret_cast<uchar *>(out.data());

    p[0] = 2;
    qToBigEndian<quint64>(sessionId, p + 1);
    qToBigEndian<quint64>(seq, p + 9);
    p[17] = 0;
    qToBigEndian<quint32>(networkId, p + 18);
    qToBigEndian<quint32>(srcVip, p + 22);
    qToBigEndian<quint32>(dstVip, p + 26);
    std::memcpy(p + 30, payload.constData(), static_cast<size_t>(payload.size()));

    return out;
}
