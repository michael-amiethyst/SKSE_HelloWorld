#include <spdlog/sinks/basic_file_sink.h>

#include <string_view>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "HeadMountedDisplay.h"
#include "MCMHelperAPI.h"
#include "NotificationService.h"
#include "Version.h"
#include "MessageTextLayout.h"

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
    constexpr std::string_view kMessageTextSetting = "sMessageText:General";
    constexpr std::string_view kSettingsChangedEvent = "HelloWorld_SettingsChanged";

    void LogMenuAssets()
    {
        const auto root = std::filesystem::absolute("Data");
        SKSE::log::info("HelloWorld {} assets: Data directory={}", HelloWorld::Version, root.string());
        for (const auto relative : {"MCM/Config/HelloWorld/config.json",
                                    "MCM/Config/HelloWorld/settings.ini",
                                    "Scripts/HelloWorldMCM.pex"}) {
            const auto path = root / relative;
            std::ifstream stream(path, std::ios::binary);
            if (!stream) {
                SKSE::log::error("Required MCM asset missing or unreadable: {}", path.string());
                continue;
            }
            const std::string bytes(std::istreambuf_iterator<char>(stream), {});
            SKSE::log::info("MCM asset {}: {} bytes", relative, bytes.size());
            if (bytes.find("sMessageText") == std::string::npos) {
                SKSE::log::error("MCM asset {} lacks sMessageText; check mod deployment/conflicts", relative);
            }
            if (std::string_view(relative).ends_with(".pex") &&
                bytes.find("HelloWorld_MessageDelayChanged") == std::string::npos) {
                SKSE::log::error("MCM script is outdated; live value callback not found");
            }
        }
        SKSE::log::info("MCM player overrides: {}",
                        (root / "MCM/Settings/HelloWorld.ini").string());
    }

    bool ApplyNotificationSettings()
    {
        if (!notificationService.ReloadSettings(mcmSettings)) {
            SKSE::log::error("Failed to load HelloWorld MCM settings; retaining current notification settings");
            return false;
        }

        SKSE::log::info("HelloWorld notification settings loaded: raw delay={} seconds, text={} bytes",
                       mcmSettings.GetModSettingInt(kMessageDelaySetting, 180),
                       mcmSettings.GetModSettingString(kMessageTextSetting, "").size());
        return true;
    }

    class SettingsChangedEventSink final : public RE::BSTEventSink<SKSE::ModCallbackEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override
        {
            if (!event) {
                return RE::BSEventNotifyControl::kContinue;
            }
            if (event->eventName == "HelloWorld_MenuOpened") {
                SKSE::log::info("MCM menu opened; script version={}", event->strArg.c_str());
                if (std::string_view(event->strArg.c_str()) != HelloWorld::Version) {
                    SKSE::log::error("MCM script/DLL version mismatch: script={}, DLL={}",
                                     event->strArg.c_str(), HelloWorld::Version);
                }
                LogMenuAssets();
                ApplyNotificationSettings();
            } else if (event->eventName == NotificationService::DelayChangedEvent ||
                       event->eventName == NotificationService::TextChangedEvent) {
                SKSE::log::info("MCM live callback: event={}, number={}, text bytes={}",
                               event->eventName.c_str(), event->numArg, event->strArg.length());
                if (!notificationService.HandleSettingChange(event->eventName.c_str(),
                                                             event->strArg.c_str(), event->numArg)) {
                    SKSE::log::error("Rejected invalid MCM callback value");
                }
                return RE::BSEventNotifyControl::kContinue;
            }
            const bool settingsChanged = event && event->eventName == kSettingsChangedEvent &&
                        (event->strArg == kMessageDelaySetting || event->strArg == kMessageTextSetting);

            if (settingsChanged) {
                SKSE::log::warn("Legacy MCM callback for {}; reload INI (check installed script version)",
                               event->strArg.c_str());
                ApplyNotificationSettings();
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    } settingsChangedEventSink;

    void InitializeLogging()
    {
        const auto logDirectory = SKSE::log::log_directory();
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
        SKSE::log::info("SKSE HelloWorld {} Initialized (live MCM callbacks)", HelloWorld::Version);
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

        const bool registerSuccess = SKSE::GetPapyrusInterface()->Register([](RE::BSScript::IVirtualMachine* vm) {
            vm->RegisterFunction("ReadMenuMessageText", "HelloWorldMCM",
                +[](RE::TESQuest*, std::string currentText) -> std::string {
                    return mcmSettings.ReloadMenuMessageText(currentText);
                });
            vm->RegisterFunction("ReadMenuMessageDelay", "HelloWorldMCM",
                +[](RE::TESQuest*, std::int32_t currentDelay) -> std::int32_t {
                    return mcmSettings.ReloadMenuMessageDelay(currentDelay);
                });
            vm->RegisterFunction("WriteMenuMessageDelay", "HelloWorldMCM",
                +[](RE::TESQuest*, std::int32_t delay) -> bool {
                    return mcmSettings.SaveMenuMessageDelay(delay);
                });
            vm->RegisterFunction("GetMessageLineCount", "HelloWorldMCM",
                +[](RE::TESQuest*, std::string text) -> std::int32_t {
                    return static_cast<std::int32_t>(MessageTextLayout::GetPreview(text).lines.size());
                });
            vm->RegisterFunction("GetMessageLine", "HelloWorldMCM",
                +[](RE::TESQuest*, std::string text, std::int32_t line) -> std::string {
                    const auto layout = MessageTextLayout::GetPreview(text);
                    return line >= 0 && static_cast<std::size_t>(line) < layout.lines.size()
                        ? layout.lines[line] : std::string{};
                });
            SKSE::log::info("MCM backing-file settings and message wrapping functions registered");
            return true;
        });
        if (!registerSuccess) {
            SKSE::log::error("Failed to register HelloWorld Papyrus callback; plugin initialization aborted");
            return false;
        }

        if (auto* eventSource = SKSE::GetModCallbackEventSource()) {
            eventSource->AddEventSink(&settingsChangedEventSink);
            SKSE::log::info("MCM callback event sink registered");
        } else {
            SKSE::log::error("SKSE mod callback event source is unavailable; MCM changes will not apply");
        }

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kDataLoaded) {
                LogMenuAssets();
                LogMCMHelperStatus();
            } else if (message->type == SKSE::MessagingInterface::kPostLoadGame && message->data != nullptr) {
                ApplyNotificationSettings();
                notificationService.Start();
            } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
                notificationService.Stop();
            }
        });

        return true;
    }
}
