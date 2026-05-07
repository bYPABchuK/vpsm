#pragma once

#include "PacketRecord.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

class PacketLogWriter {
public:
    PacketLogWriter();
    ~PacketLogWriter();

    bool start(const std::string &filePath);
    void stop();
    void enqueue(const PacketRecord &record);

private:
    void workerLoop();

    std::atomic<bool> m_running {false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<PacketRecord> m_queue;
    std::string m_path;
};
