#include <catch2/catch_test_macros.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include "NotificationService.h"
#include "MCMHelperAPI.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
    using Clock = std::chrono::steady_clock;

    class ScopedLogCapture
    {
    public:
        ScopedLogCapture() : previous_(spdlog::default_logger())
        {
            auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(output_);
            auto logger = std::make_shared<spdlog::logger>("expected-test-error", sink);
            logger->set_pattern("%l: %v");
            spdlog::set_default_logger(std::move(logger));
        }

        ~ScopedLogCapture() { spdlog::set_default_logger(previous_); }

        ScopedLogCapture(const ScopedLogCapture&) = delete;
        ScopedLogCapture& operator=(const ScopedLogCapture&) = delete;

        std::string Text() const { return output_.str(); }

    private:
        std::ostringstream output_;
        std::shared_ptr<spdlog::logger> previous_;
    };

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

TEST_CASE("NotificationService overrides every transport and restores defaults for blank text",
          "[NotificationService]")
{
    for (const auto transport : {HeadMountedDisplay::Transport::kWired,
                                 HeadMountedDisplay::Transport::kWireless,
                                 HeadMountedDisplay::Transport::kUnknown}) {
        NotificationProbe probe;
        NotificationService service([transport] { return transport; },
            [&probe](std::string text) { probe.Record(std::move(text)); });
        service.SetMessageDelay(20ms);
        service.SetMessageText("  Custom message!  ");
        service.Start();
        REQUIRE(probe.WaitForCount(1, 1s));
        service.Stop();
        CHECK(probe.At(0).message == "  Custom message!  ");
        for (const std::string text : {"", " \t\r\n\f\v"}) {
            service.SetMessageText(text);
            const auto count = probe.Count();
            service.Start();
            REQUIRE(probe.WaitForCount(count + 1, 1s));
            service.Stop();
            CHECK(probe.At(count).message == NotificationService::GetMessage_(transport));
        }
    }
}

TEST_CASE("NotificationService keeps one active timer after message text updates",
          "[NotificationService][single-timer]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    constexpr auto interval = 200ms;
    service.SetMessageDelay(interval);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 2s));

    SECTION("one text update")
    {
        // Stagger the update from the original timer so an additional timer would
        // produce notifications less than one full interval apart.
        REQUIRE_FALSE(probe.WaitForCount(2, 60ms));
        service.SetMessageText("Updated message");
    }
    SECTION("successive text updates including restoring defaults")
    {
        REQUIRE_FALSE(probe.WaitForCount(2, 30ms));
        service.SetMessageText("First update");
        REQUIRE_FALSE(probe.WaitForCount(2, 30ms));
        service.SetMessageText("Second update");
        service.SetMessageText("");
        service.SetMessageText("Updated message");
    }

    // Observe several cycles: checking just the first updated notification would
    // miss a second timer that starts emitting slightly later.
    for (std::size_t index = 1; index <= 4; ++index) {
        REQUIRE(probe.WaitForCount(index + 1, 2s));
        CHECK(probe.At(index).message == "Updated message");
        CHECK(probe.At(index).time - probe.At(index - 1).time >= interval);
        REQUIRE_FALSE(probe.WaitForCount(index + 2, 60ms));
    }
    service.Stop();
    CHECK(probe.Count() == 5);
    CHECK_FALSE(probe.WaitForCount(6, interval * 2));
}

TEST_CASE("Live MCM delay callback replaces the current countdown",
          "[NotificationService][MCMHelperAPI][integration]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(30ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 1s));
    REQUIRE(service.HandleSettingChange(NotificationService::DelayChangedEvent, "iMessageDelay:General", 600));
    const auto count = probe.Count();
    CHECK_FALSE(probe.WaitForCount(count + 1, 200ms));
    CHECK_FALSE(service.HandleSettingChange(NotificationService::DelayChangedEvent, "",
                                           std::numeric_limits<float>::quiet_NaN()));
    CHECK_FALSE(service.HandleSettingChange("UnrelatedEvent", "", 10));
    CHECK_FALSE(probe.WaitForCount(count + 1, 100ms));
    service.Stop();
}

TEST_CASE("Live MCM text callback updates a single running timer without an INI reload",
          "[NotificationService][MCMHelperAPI][integration]")
{
    NotificationProbe probe;
    auto service = MakeService(probe);
    service.SetMessageDelay(50ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 1s));
    for (const std::string text : {"Live text", "", "   ", "Replacement"}) {
        REQUIRE(service.HandleSettingChange(NotificationService::TextChangedEvent, text, 0));
        const auto count = probe.Count();
        REQUIRE(probe.WaitForCount(count + 1, 1s));
        CHECK(probe.At(count).message == (text.find_first_not_of(' ') == std::string::npos
            ? NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWireless) : text));
        CHECK(probe.At(count).time - probe.At(count - 1).time >= 50ms);
    }
    service.Stop();
}

TEST_CASE("MCM Helper reloads notification text while NotificationService is running",
          "[NotificationService][MCMHelperAPI][integration]")
{
    struct TestDirectory {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("HelloWorld-Notifications-" + std::to_string(Clock::now().time_since_epoch().count()));
        ~TestDirectory() { std::filesystem::remove_all(path); }
    } data;
    const auto defaults = data.path / "MCM/Config/HelloWorld/settings.ini";
    const auto overrides = data.path / "MCM/Settings/HelloWorld.ini";
    std::filesystem::create_directories(defaults.parent_path());
    std::filesystem::create_directories(overrides.parent_path());
    std::ofstream(defaults) << "[General]\niMessageDelay=180\nsMessageText=\n";
    MCMHelperAPI settings("HelloWorld", data.path);
    NotificationProbe probe;
    auto service = MakeService(probe);
    REQUIRE(service.ReloadSettings(settings));
    service.SetMessageDelay(20ms);
    service.Start();
    REQUIRE(probe.WaitForCount(1, 1s));
    CHECK(probe.At(0).message == NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWireless));

    for (const std::string text : {"Custom courier", "Replacement", "", "   "}) {
        std::ofstream(overrides) << "[General]\nsMessageText=" << text << '\n';
        const auto displayedText = settings.ReloadMenuMessageText("stale menu value");
        REQUIRE(service.ReloadSettings(settings));
        // Production clamps delays to at least ten seconds; accelerate only the timer for this test.
        service.SetMessageDelay(20ms);
        const auto count = probe.Count();
        REQUIRE(probe.WaitForCount(count + 1, 1s));
        CHECK(probe.At(count).message == (text.find_first_not_of(' ') == std::string::npos
            ? NotificationService::GetMessage_(HeadMountedDisplay::Transport::kWireless) : text));
        CHECK(displayedText == (text.find_first_not_of(' ') == std::string::npos ? "" : text));
    }
    service.Stop();

    std::filesystem::remove(overrides);
    REQUIRE(service.ReloadSettings(settings));
    CHECK(settings.GetModSettingString("sMessageText:General", "missing").empty());
    // An unreadable INI must report failure rather than applying partial defaults.
    std::filesystem::remove(defaults);
    std::filesystem::create_directory(defaults);
    {
        // Capture only the deliberately induced failure, restoring normal logging
        // on scope exit even when an assertion throws.
        ScopedLogCapture logs;
        CHECK_FALSE(service.ReloadSettings(settings));
        CHECK(settings.ReloadMenuMessageText("last displayed text") == "last displayed text");
        CHECK(settings.ReloadMenuMessageDelay(120) == 120);
        std::filesystem::create_directory(overrides);
        CHECK_FALSE(settings.SaveMenuMessageDelay(60));
        INFO(logs.Text());
        CHECK(logs.Text().find("error: MCM settings file cannot be opened: ") != std::string::npos);
        CHECK(logs.Text().find(defaults.filename().string()) != std::string::npos);
    }
}
