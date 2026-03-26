#include "../../src/application/DataPlane/MetricService.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
    using vpsm::server::application::MetricService;
    using vpsm::server::domain::MetricsSnapshot;
    using vpsm::server::domain::RouteAction;
    using vpsm::server::port::METRIC_FLUSH_OK;
    using vpsm::server::port::MetricFlushErrors;

    class MetricCounterFake final : public vpsm::server::port::IMetricCounter {
    public:
        void routeActionCount(RouteAction) override {}
        void usersOnlineCount(RouteAction) override {}

        MetricsSnapshot snapshotAndReset() override {
            ++snapshotAndResetCalls;
            return snapshotToReturn;
        }

        MetricsSnapshot snapshotWithoutReset() override {
            return snapshotToReturn;
        }

        MetricsSnapshot snapshotToReturn{};
        int snapshotAndResetCalls = 0;
    };

    class MetricsSinkFake final : public vpsm::server::port::IMetricsSink {
    public:
        MetricFlushErrors flush(const MetricsSnapshot& snapshot) override {
            {
                std::lock_guard<std::mutex> lock(mutex);
                ++flushCalls;
                lastSnapshot = snapshot;
                firstResult = resultToReturn;
            }
            cv.notify_all();
            return resultToReturn;
        }

        MetricFlushErrors resultToReturn = METRIC_FLUSH_OK;
        int flushCalls = 0;
        MetricsSnapshot lastSnapshot{};
        MetricFlushErrors firstResult = METRIC_FLUSH_OK;

        std::mutex mutex;
        std::condition_variable cv;
    };

    TEST(MetricServiceTest, start_OneCycle_InvokesSinkFlushMetricFlushOkEnumValue) {

        MetricCounterFake counter;
        counter.snapshotToReturn = MetricsSnapshot{
            .packetsReceived = 10,
            .packetsForwarded = 20,
            .packetsDropped = 30,
            .packetsResponded = 40,
            .usersOnline = 50,
        };
        MetricsSinkFake sink;
        sink.resultToReturn = METRIC_FLUSH_OK;
        MetricService service(counter, sink);

        std::thread worker([&service]() {
            service.start();
        });

        {
            std::unique_lock<std::mutex> lock(sink.mutex);
            const bool flushed = sink.cv.wait_for(
                lock,
                std::chrono::seconds(3),
                [&sink]() { return sink.flushCalls > 0; }
            );
            ASSERT_TRUE(flushed);
            EXPECT_EQ(sink.firstResult, METRIC_FLUSH_OK);
            EXPECT_EQ(sink.lastSnapshot.packetsReceived, 10u);
        }

        service.stop();
        worker.join();
    }

    TEST(MetricServiceTest, stop_AfterStart_ServiceThreadStopsTrue) {

        MetricCounterFake counter;
        MetricsSinkFake sink;
        MetricService service(counter, sink);

        std::thread worker([&service]() {
            service.start();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        service.stop();
        worker.join();

        SUCCEED();
    }

}
