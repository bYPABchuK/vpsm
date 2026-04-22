#include "../../src/adapter/FileMetricsSink.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {
    using vpsm::server::adapter::FileMetricsSink;
    using vpsm::server::domain::MetricsSnapshot;
    using vpsm::server::port::METRIC_FLUSH_OK;
    using vpsm::server::port::METRIC_FLUSH_OPEN_TMP_FAILED;

    std::filesystem::path makeTempFilePath(const std::string& suffix) {
        auto path = std::filesystem::temp_directory_path();
        path /= ("vpsm_metrics_" + suffix + ".txt");
        return path;
    }

    TEST(FileMetricsSinkTest, flush_ValidPath_ReturnsMetricFlushOkEnumValue) {

        const auto path = makeTempFilePath("flush_valid_path_ok");
        FileMetricsSink sink(path);
        MetricsSnapshot snapshot{
            .packetsReceived = 1,
            .packetsForwarded = 2,
            .packetsDropped = 3,
            .packetsDroppedParse = 4,
            .packetsDroppedAuth = 5,
            .packetsDroppedMembership = 6,
            .packetsDroppedNoEndpoint = 7,
            .packetsResponded = 4,
            .usersOnline = 5,
        };

        const auto result = sink.flush(snapshot);

        EXPECT_EQ(result, METRIC_FLUSH_OK);
        std::filesystem::remove(path);
    }

    TEST(FileMetricsSinkTest, flush_InvalidDirectory_ReturnsMetricFlushOpenTmpFailedEnumValue) {

        const std::filesystem::path path = "/definitely_missing_dir_for_vpsm_tests/file.txt";
        FileMetricsSink sink(path);
        MetricsSnapshot snapshot{};

        const auto result = sink.flush(snapshot);

        EXPECT_EQ(result, METRIC_FLUSH_OPEN_TMP_FAILED);
    }

    TEST(FileMetricsSinkTest, flush_ValidPath_WritesExpectedMetricsTextTrue) {

        const auto path = makeTempFilePath("flush_content_true");
        FileMetricsSink sink(path);
        MetricsSnapshot snapshot{
            .packetsReceived = 11,
            .packetsForwarded = 22,
            .packetsDropped = 33,
            .packetsDroppedParse = 44,
            .packetsDroppedAuth = 55,
            .packetsDroppedMembership = 66,
            .packetsDroppedNoEndpoint = 77,
            .packetsResponded = 44,
            .usersOnline = 55,
        };

        ASSERT_EQ(sink.flush(snapshot), METRIC_FLUSH_OK);

        std::ifstream in(path);
        ASSERT_TRUE(in.is_open());
        const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        EXPECT_NE(content.find("packets_received 11"), std::string::npos);
        EXPECT_NE(content.find("packets_forwarded 22"), std::string::npos);
        EXPECT_NE(content.find("packets_dropped 33"), std::string::npos);
        EXPECT_NE(content.find("packets_dropped_parse 44"), std::string::npos);
        EXPECT_NE(content.find("packets_dropped_auth 55"), std::string::npos);
        EXPECT_NE(content.find("packets_dropped_membership 66"), std::string::npos);
        EXPECT_NE(content.find("packets_dropped_no_endpoint 77"), std::string::npos);
        EXPECT_NE(content.find("packets_responded 44"), std::string::npos);
        EXPECT_NE(content.find("users_online 55"), std::string::npos);

        std::filesystem::remove(path);
    }

}
