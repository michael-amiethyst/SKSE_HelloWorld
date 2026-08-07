#include <spdlog/sinks/basic_file_sink.h>

namespace
{
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
    }

    class MainMenuListener final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        static MainMenuListener* GetSingleton()
        {
            static MainMenuListener singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::MenuOpenCloseEvent* event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (event && event->opening && event->menuName == RE::MainMenu::MENU_NAME) {
                SKSE::log::info("Main Menu opened; queuing Hello World notification");

                // The open event can be emitted while the menu is still finishing
                // initialization. Run on the next UI update after that event.
                SKSE::GetTaskInterface()->AddUITask([] {
                    const auto ui = RE::UI::GetSingleton();
                    if (!ui || !ui->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
                        SKSE::log::warn("Main Menu was no longer open when the UI task ran");
                        return;
                    }

                    SKSE::log::info("UI task running with Main Menu open; sending notification");
                    RE::DebugNotification("Hello, world!");
                    SKSE::log::info("DebugNotification returned");
                });
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    InitializeLogging();
    SKSE::log::info("HelloWorld loaded");

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kInputLoaded) {
            if (auto ui = RE::UI::GetSingleton()) {
                ui->AddEventSink(MainMenuListener::GetSingleton());
                SKSE::log::info("Registered Main Menu open listener");
            } else {
                SKSE::log::error("UI singleton was unavailable at kInputLoaded");
            }
        } else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            SKSE::log::info("Received kDataLoaded");
        }
    });

    return true;
}
