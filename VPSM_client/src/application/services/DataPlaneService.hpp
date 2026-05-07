#pragma once

#include "PacketRecord.hpp"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class DataPlaneService final : public QObject {
    Q_OBJECT
public:
    explicit DataPlaneService(QObject *parent = nullptr);
    ~DataPlaneService() override;

    bool start(int txThreads,
               int rxThreads,
               quint16 listenPort,
               const QString &routerHost,
               quint16 routerPort);
    void stop();
    bool enqueueSend(const QString &srcVip,
                     const QString &destVip,
                     const QString &networkIp,
                     const QString &payload);

signals:
    void packetSent(const PacketRecord &record);
    void packetReceived(const PacketRecord &record);
    void errorOccurred(const QString &message);

private:
    struct SendTask {
        QString srcVip;
        QString destVip;
        QString networkIp;
        QString payload;
    };

    void txLoop();
    void rxLoop();
    static bool parseIpv4(const QString &value, quint32 &out);
    static QString ipv4ToString(quint32 value);
    static QByteArray buildPacket(quint64 seq,
                                  quint64 sessionId,
                                  quint32 networkId,
                                  quint32 srcVip,
                                  quint32 dstVip,
                                  const QByteArray &payload);

    std::atomic<bool> m_running {false};
    std::atomic<quint64> m_nextSeq {1};
    quint64 m_sessionId {1};

    QString m_routerHost;
    quint16 m_routerPort {0};
    quint16 m_listenPort {0};

    std::mutex m_txMutex;
    std::condition_variable m_txCv;
    std::deque<SendTask> m_txQueue;
    std::vector<std::thread> m_txWorkers;
    std::vector<std::thread> m_rxWorkers;
};
