#include <spdlog/sinks/basic_file_sink.h>

#include "HeadMountedDisplay.h"
#include "MCMHelperAPI.h"
#include "NotificationService.h"

namespace
{
    HeadMountedDisplay transportDetector;
    NotificationService notificationService(transportDetector);

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

    void LogMCMHelperStatus(const SKSE::LoadInterface& skse)
    {
        const auto version = MCMHelperAPI::GetRuntimeVersion(skse);
        if (!version) {
            SKSE::log::error(
                "Required dependency MCM Helper was not found. Install the Skyrim VR build of "
                "MCM Helper 1.4.0 or later as a separate mod; the HelloWorld MCM will not load "
                "without it.");
        } else if (*version < MCMHelperAPI::MinimumVersion) {
            SKSE::log::error(
                "MCM Helper version code {} is loaded, but HelloWorld requires version code {} "
                "(MCM Helper 1.4.0 or later). The HelloWorld MCM may not load.",
                *version,
                MCMHelperAPI::MinimumVersion);
        } else {
            SKSE::log::info("MCM Helper version code {} found", *version);
        }
    }

    /** Plugin's main */
    SKSEPluginLoad(const SKSE::LoadInterface *skse) {
        SKSE::Init(skse);
        InitializeLogging();
        LogMCMHelperStatus(*skse);

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kPostLoadGame && message->data != nullptr) {
                notificationService.Start();
            } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
                notificationService.Stop();
            }
        });

        return true;
    }
}
