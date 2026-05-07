#include "PacketLogWriter.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <chrono>
#include <fstream>

PacketLogWriter::PacketLogWriter() = default;

PacketLogWriter::~PacketLogWriter()
{
    stop();
}

bool PacketLogWriter::start(const std::string &filePath)
{
    if (m_running.exchange(true)) {
        return false;
    }
    m_path = filePath;
    m_thread = std::thread(&PacketLogWriter::workerLoop, this);
    return true;
}

void PacketLogWriter::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }
    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void PacketLogWriter::enqueue(const PacketRecord &record)
{
    if (!m_running.load()) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(record);
    }
    m_cv.notify_one();
}

void PacketLogWriter::workerLoop()
{
    std::ofstream out(m_path, std::ios::app);
    while (m_running.load() || !m_queue.empty()) {
        PacketRecord item;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(200), [this] {
                return !m_running.load() || !m_queue.empty();
            });

            if (m_queue.empty()) {
                continue;
            }
            item = m_queue.front();
            m_queue.pop_front();
        }

        QJsonObject root {
            {"timestamp", item.timestampIso},
            {"seq", static_cast<qint64>(item.seq)},
            {"srcVip", item.srcVip},
            {"destVip", item.destVip},
            {"networkIp", item.networkIp}
        };

        const QByteArray line = QJsonDocument(root).toJson(QJsonDocument::Compact);
        out << line.constData() << '\n';
        out.flush();
    }
}
