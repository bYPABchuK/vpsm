#include "FileMetricsSink.hpp"
#include <string>
#include <fcntl.h>
#include <unistd.h>
namespace vpsm::server::adapter {
    static bool writeAll(int fd, const char* data, std::size_t size) {
        std::size_t pos = 0;
        while (pos < size) {
            const ssize_t n = ::write(fd, data + pos, size - pos);
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            pos += static_cast<std::size_t>(n);
        }
        return true;
    }

    port::MetricFlushErrors FileMetricsSink::flush(const domain::MetricsSnapshot& s) {
        auto tmp = output_;
        tmp += ".tmp";

        std::string payload;
        payload.reserve(320);
        payload += "packets_received ";  payload += std::to_string(s.packetsReceived);  payload += '\n';
        payload += "packets_forwarded "; payload += std::to_string(s.packetsForwarded); payload += '\n';
        payload += "packets_dropped ";   payload += std::to_string(s.packetsDropped);   payload += '\n';
        payload += "packets_dropped_parse "; payload += std::to_string(s.packetsDroppedParse); payload += '\n';
        payload += "packets_dropped_auth "; payload += std::to_string(s.packetsDroppedAuth); payload += '\n';
        payload += "packets_dropped_membership "; payload += std::to_string(s.packetsDroppedMembership); payload += '\n';
        payload += "packets_dropped_no_endpoint "; payload += std::to_string(s.packetsDroppedNoEndpoint); payload += '\n';
        payload += "packets_responded "; payload += std::to_string(s.packetsResponded); payload += '\n';
        payload += "users_online ";      payload += std::to_string(s.usersOnline);      payload += '\n';

        int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        if (fd < 0) {
            return port::METRIC_FLUSH_OPEN_TMP_FAILED;
        }

        if (!writeAll(fd, payload.data(), payload.size())) {
            ::close(fd);
            return port::METRIC_FLUSH_WRITE_FAILED;
        }

        if (::fsync(fd) != 0) {
            ::close(fd);
            return port::METRIC_FLUSH_FSYNC_FILE_FAILED;
        }

        ::close(fd);

        if (::rename(tmp.c_str(), output_.c_str()) != 0) {
            return port::METRIC_FLUSH_RENAME_FAILED;
        }

        const auto parent = output_.parent_path().empty()
            ? std::filesystem::path(".")
            : output_.parent_path();

        int dfd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dfd < 0) {
            return port::METRIC_FLUSH_OPEN_DIR_FAILED;
        }

        if (::fsync(dfd) != 0) {
            ::close(dfd);
            return port::METRIC_FLUSH_FSYNC_DIR_FAILED;
        }

        ::close(dfd);
        return port::METRIC_FLUSH_OK;
    }
}
