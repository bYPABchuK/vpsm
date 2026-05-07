#pragma once

#include <boost/json/object.hpp>

#include <chrono>
#include <filesystem>
#include <mutex>

namespace vpsm::server::application {
    class UiScreenService {
    public:
        enum class Status {
            Ok,
            NotFound,
            Error,
        };

        struct Result {
            Status status = Status::Error;
            boost::json::object payload;
        };

        UiScreenService(
            std::filesystem::path mainBodyPath,
            std::filesystem::path licensescreenPath,
            std::chrono::milliseconds checkInterval = std::chrono::milliseconds(1000)
        );

        Result getMainBody();
        Result getLicensescreen();

    private:
        struct CacheEntry {
            std::filesystem::path path;
            std::chrono::steady_clock::time_point lastCheckedAt{};
            std::filesystem::file_time_type lastWriteTime{};
            bool hasLastWriteTime = false;
            bool hasCache = false;
            Result cached;
        };

        Result read(CacheEntry& entry);
        static Result readFromDisk(const std::filesystem::path& path);

        std::mutex mutex_;
        CacheEntry mainBody_;
        CacheEntry licensescreen_;
        std::chrono::milliseconds checkInterval_;
    };
}
