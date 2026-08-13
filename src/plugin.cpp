#include <spdlog/sinks/basic_file_sink.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace
{
    using namespace std::chrono_literals;

    std::jthread notificationThread;
    std::condition_variable_any notificationTimer;
    std::mutex notificationMutex;

    void StopNotifications()
    {
        notificationThread.request_stop();
        notificationTimer.notify_all();
        if (notificationThread.joinable()) {
            notificationThread.join();
        }
    }

    void StartNotifications()
    {
        StopNotifications();
        notificationThread = std::jthread([](std::stop_token stopToken) {
            std::unique_lock lock(notificationMutex);

            while (!stopToken.stop_requested()) {
                notificationTimer.wait_for(lock, stopToken, 10s, [] { return false; });
                if (stopToken.stop_requested()) {
                    break;
                }

                SKSE::GetTaskInterface()->AddUITask([] {
                    RE::DebugNotification("Hello Talos!");
                    SKSE::log::info("Recurring DebugNotification called after kPostLoadGame");
                });
            }
        });
    }

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
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
        SKSE::log::info("SKSE HelloWorld Initialized");
    }

    SKSEPluginLoad(const SKSE::LoadInterface *skse) {
        SKSE::Init(skse);
        InitializeLogging();

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
            if (message->type == SKSE::MessagingInterface::kPostLoadGame && message->data != nullptr) {
                StartNotifications();
            } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
                StopNotifications();
            }
        });

        return true;
    }
}
