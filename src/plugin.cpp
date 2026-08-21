#include <spdlog/sinks/basic_file_sink.h>

#include "HeadMountedDisplay.h"
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

    /** Plugin's main */
    SKSEPluginLoad(const SKSE::LoadInterface *skse) {
        SKSE::Init(skse);
        InitializeLogging();

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
