#include <catch2/catch_test_macros.hpp>

#include "NotificationService.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    using Clock = std::chrono::steady_clock;

    class NotificationProbe
    {
    public:
        struct Notification
        {
            std::string message;
            Clock::time_point time;
        };

        void Record(std::string message)
        {
            {
                std::scoped_lock lock(mutex_);
                notifications_.push_back({std::move(message), Clock::now()});
            }
            changed_.notify_all();
        }

        bool WaitForCount(std::size_t count, Clock::duration timeout)
        {
            std::unique_lock lock(mutex_);
            return changed_.wait_for(lock, timeout, [&] { return notifications_.size() >= count; });
        }

        std::size_t Count() const
        {
            std::scoped_lock lock(mutex_);
            return notifications_.size();
        }

        Notification At(std::size_t index) const
        {
            std::scoped_lock lock(mutex_);
            return notifications_.at(index);
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable changed_;
        std::vector<Notification> notifications_;
    };

    NotificationService MakeService(NotificationProbe& probe)
    {
        return NotificationService(
            [] { return HeadMountedDisplay::Transport::kWireless; },
            [&probe](std::string message) { probe.Record(std::move(message)); });
    }
}

TEST_CASE("NotificationService selects the wired transport message", "[NotificationService]")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWired)) ==
          "Zenithar's courier travels through the earth.");
}

TEST_CASE("NotificationService selects the wireless transport message", "[NotificationService]")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWireless)) ==
          "Zenithar's courier travels through the air.");
}

TEST_CASE("NotificationService selects the fallback transport message", "[NotificationService]")
{
    CHECK(std::string_view(NotificationService::GetMessage_(HeadMountedDisplay::Transport::kUnknown)) ==
          "Zenithar's courier is confused");
}

TEST_CASE("NotificationService uses a delay configured before starting and repeats notifications",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(100ms);
    const auto started = Clock::now();
    service.Start();

    CHECK_FALSE(probe.WaitForCount(1, 30ms));
    REQUIRE(probe.WaitForCount(2, 2s));
    service.Stop();

    CHECK(probe.At(0).message == "Zenithar's courier travels through the air.");
    CHECK(probe.At(1).message == probe.At(0).message);
    CHECK(probe.At(0).time - started >= 100ms);
    CHECK(probe.At(1).time - probe.At(0).time >= 100ms);
}

TEST_CASE("NotificationService can shorten the default countdown while it is running",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.Start();
    CHECK_FALSE(probe.WaitForCount(1, 50ms));

    const auto changed = Clock::now();
    service.SetMessageDelay(30ms);
    REQUIRE(probe.WaitForCount(1, 1s));
    service.Stop();
    CHECK(probe.At(0).time - changed >= 30ms);
}

TEST_CASE("NotificationService replaces an active countdown with longer and shorter delays",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(100ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 2s));

    const auto lengthened = Clock::now();
    service.SetMessageDelay(1s);
    // A wake-up must restart the countdown rather than finish the old 100 ms wait.
    CHECK_FALSE(probe.WaitForCount(2, 300ms));
    REQUIRE(probe.WaitForCount(2, 2s));
    CHECK(probe.At(1).time - lengthened >= 1s);

    const auto shortened = Clock::now();
    service.SetMessageDelay(30ms);
    REQUIRE(probe.WaitForCount(3, 500ms));
    service.Stop();
    CHECK(probe.At(2).time - shortened >= 30ms);
}

TEST_CASE("NotificationService applies the most recent of successive delay changes",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.Start();
    CHECK_FALSE(probe.WaitForCount(1, 50ms));

    service.SetMessageDelay(600s);
    service.SetMessageDelay(300s);
    service.SetMessageDelay(30ms);
    REQUIRE(probe.WaitForCount(1, 1s));
    service.Stop();
}

TEST_CASE("NotificationService stops a long countdown promptly and Stop is repeatable",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(600s);
    service.Start();
    CHECK_FALSE(probe.WaitForCount(1, 50ms));

    const auto stopping = Clock::now();
    service.Stop();
    CHECK(Clock::now() - stopping < 500ms);
    service.Stop();
    CHECK_FALSE(probe.WaitForCount(1, 50ms));
}

TEST_CASE("NotificationService uses a setting changed while stopped on the next Start",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(600s);
    service.Start();
    service.Stop();

    service.SetMessageDelay(30ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 1s));
    service.Stop();
    const auto stoppedCount = probe.Count();
    CHECK_FALSE(probe.WaitForCount(stoppedCount + 1, 100ms));
}

TEST_CASE("NotificationService restarts an active service with a fresh countdown",
          "[NotificationService]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(200ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 2s));
    CHECK_FALSE(probe.WaitForCount(2, 100ms));

    const auto restarted = Clock::now();
    service.Start();
    CHECK_FALSE(probe.WaitForCount(2, 120ms));
    REQUIRE(probe.WaitForCount(2, 2s));
    service.Stop();
    CHECK(probe.At(1).time - restarted >= 200ms);
}

TEST_CASE("NotificationService destruction stops further notifications", "[NotificationService]")
{
    NotificationProbe probe;
    {
        auto service = MakeService(probe);
        service.SetMessageDelay(30ms);
        service.Start();
        REQUIRE(probe.WaitForCount(1, 1s));
    }
    const auto stoppedCount = probe.Count();
    CHECK_FALSE(probe.WaitForCount(stoppedCount + 1, 100ms));
}
