#include "UiScreenService.hpp"

#include <boost/json/parse.hpp>
#include <boost/system/error_code.hpp>

#include <fstream>
#include <string>
#include <utility>

namespace vpsm::server::application {
    UiScreenService::UiScreenService(
        std::filesystem::path mainBodyPath,
        std::filesystem::path licensescreenPath,
        std::chrono::milliseconds checkInterval
    )
        : mainBody_{.path = std::move(mainBodyPath)},
          licensescreen_{.path = std::move(licensescreenPath)},
          checkInterval_(checkInterval) {}

    UiScreenService::Result UiScreenService::getMainBody() {
        std::lock_guard lock(mutex_);
        return read(mainBody_);
    }

    UiScreenService::Result UiScreenService::getLicensescreen() {
        std::lock_guard lock(mutex_);
        return read(licensescreen_);
    }

    UiScreenService::Result UiScreenService::read(CacheEntry& entry) {
        const auto now = std::chrono::steady_clock::now();
        if (entry.hasCache && (now - entry.lastCheckedAt) < checkInterval_) {
            return entry.cached;
        }
        entry.lastCheckedAt = now;

        boost::system::error_code ec;
        const bool exists = std::filesystem::exists(entry.path, ec);
        if (ec || !exists) {
            entry.cached = Result{.status = Status::NotFound};
            entry.hasCache = true;
            entry.hasLastWriteTime = false;
            return entry.cached;
        }

        const auto writeTime = std::filesystem::last_write_time(entry.path, ec);
        if (ec) {
            entry.cached = Result{.status = Status::Error};
            entry.hasCache = true;
            entry.hasLastWriteTime = false;
            return entry.cached;
        }

        if (entry.hasCache && entry.hasLastWriteTime && entry.lastWriteTime == writeTime) {
            return entry.cached;
        }

        entry.cached = readFromDisk(entry.path);
        entry.hasCache = true;
        entry.lastWriteTime = writeTime;
        entry.hasLastWriteTime = true;
        return entry.cached;
    }

    UiScreenService::Result UiScreenService::readFromDisk(const std::filesystem::path& path) {
        boost::system::error_code ec;
        const auto fileSize = std::filesystem::file_size(path, ec);
        if (ec) {
            return Result{.status = Status::Error};
        }

        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            return Result{.status = Status::NotFound};
        }

        std::string text;
        text.resize(static_cast<std::size_t>(fileSize));
        if (!text.empty()) {
            in.read(text.data(), static_cast<std::streamsize>(text.size()));
        }
        if (!in.good() && !in.eof()) {
            return Result{.status = Status::Error};
        }

        boost::system::error_code parseEc;
        const auto jsonValue = boost::json::parse(text, parseEc);
        if (parseEc || !jsonValue.is_object()) {
            return Result{.status = Status::Error};
        }

        return Result{
            .status = Status::Ok,
            .payload = jsonValue.as_object(),
        };
    }
}
