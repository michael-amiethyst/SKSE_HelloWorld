#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <chrono>
#include <string_view>

#include "HeadMountedDisplay.h"
#include "MCMHelperAPI.h"
#include "NotificationService.h"

namespace
{
    HeadMountedDisplay transportDetector;
    NotificationService notificationService(
        [] { return transportDetector.GetTransport(); },
        [](std::string message) {
            SKSE::GetTaskInterface()->AddUITask([message = std::move(message)] {
                RE::DebugNotification(message.c_str());
                SKSE::log::info("{}", message);
            });
        });
    MCMHelperAPI mcmSettings("HelloWorld");

    constexpr std::string_view kMessageDelaySetting = "iMessageDelay:General";
    constexpr std::string_view kSettingsChangedEvent = "HelloWorld_SettingsChanged";

    bool ApplyMessageDelaySetting()
    {
        if (!mcmSettings.Reload()) {
            SKSE::log::error("Failed to load HelloWorld MCM settings; using the default delay");
            return false;
        }

        const auto configuredDelay = mcmSettings.GetModSettingInt(kMessageDelaySetting, 180);
        const auto delay = std::clamp(configuredDelay, 10, 600);
        notificationService.SetMessageDelay(std::chrono::seconds{delay});
        SKSE::log::info("HelloWorld notification delay set to {} seconds", delay);
        return true;
    }

    class SettingsChangedEventSink final : public RE::BSTEventSink<SKSE::ModCallbackEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const SKSE::ModCallbackEvent* event,
            RE::BSTEventSource<SKSE::ModCallbackEvent>*) override
        {
            if (event && event->eventName == kSettingsChangedEvent && event->strArg == kMessageDelaySetting) {
                ApplyMessageDelaySetting();
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    } settingsChangedEventSink;

    void InitializeLogging()
    {
        auto logDirectory = SKSE::log::log_directory();
        if (!logDirectory) {
            throw std::runtime_error("SKSE log directory is unavailable");
        }

        const auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        const auto logPath = *logDirectory / (std::string(pluginName) + ".log");
        auto logger = spdlog::basic_logger_mt("global", logPath.string(), true);

        spdlog::set_default_logger(std::move(logger));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::debug);
        SKSE::log::info("SKSE HelloWorld Initialized");
    }

    void LogMCMHelperStatus()
    {
        MCMHelperAPI::GetRuntimeVersion([](const std::optional<std::uint32_t> version) {
            if (!version) {
                SKSE::log::error(
                    "Required dependency MCM Helper was not found. Install the Skyrim VR build "
                    "of MCM Helper 1.4.0 or later as a separate mod; the HelloWorld MCM will not "
                    "load without it.");
            } else if (*version < MCMHelperAPI::MinimumVersion) {
                SKSE::log::error(
                    "MCM Helper version code {} is loaded, but HelloWorld requires version code "
                    "{} (MCM Helper 1.4.0 or later). The HelloWorld MCM may not load.",
                    *version,
                    MCMHelperAPI::MinimumVersion);
            } else {
                SKSE::log::info("MCM Helper version code {} found", *version);
            }
        });
    }

    /** Plugin's main */
    SKSEPluginLoad(const SKSE::LoadInterface *skse) {
        SKSE::Init(skse);
        InitializeLogging();

        if (auto* eventSource = SKSE::GetModCallbackEventSource()) {
            eventSource->AddEventSink(&settingsChangedEventSink);
        } else {
            SKSE::log::error("SKSE mod callback event source is unavailable; MCM changes will not apply");
        }

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                LogMCMHelperStatus();
            } else if (message->type == SKSE::MessagingInterface::kPostLoadGame && message->data != nullptr) {
                ApplyMessageDelaySetting();
                notificationService.Start();
            } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
                notificationService.Stop();
            }
        });

        return true;
    }
}
