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
        SKSE::log::info("SKSE HelloWorld Initialized");
    }

    class LoadingMenuListener final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        static LoadingMenuListener* GetSingleton()
        {
            static LoadingMenuListener singleton;
            return &singleton;
        }

        void NotifyAfterLoad()
        {
            notificationPending = true;
            SKSE::log::info("Notification armed; waiting for Loading Menu to close");
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::MenuOpenCloseEvent* event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (event && !event->opening && event->menuName == RE::LoadingMenu::MENU_NAME &&
                notificationPending.exchange(false)) {
                SKSE::log::info("Loading Menu closed; queuing notification on UI thread");
                SKSE::GetTaskInterface()->AddUITask([] {
                    RE::DebugNotification("Hello Talos!");
                    SKSE::log::info("DebugNotification called after Loading Menu closed");
                });
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        std::atomic_bool notificationPending{false};
    };

    SKSEPluginLoad(const SKSE::LoadInterface *skse) {
        SKSE::Init(skse);
        InitializeLogging();

        SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
            if (message->type == SKSE::MessagingInterface::kInputLoaded) {
                if (auto ui = RE::UI::GetSingleton()) {
                    ui->AddEventSink(LoadingMenuListener::GetSingleton());
                    SKSE::log::info("Registered Loading Menu listener");
                } else {
                    SKSE::log::error("UI singleton unavailable at kInputLoaded");
                }
            } else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
                // SKSE stores the success flag in the pointer value itself; it is
                // not a pointer to a bool and must never be dereferenced.
                const auto loadSucceeded = message->data != nullptr;
                if (!loadSucceeded) {
                    SKSE::log::warn("Save-game load failed; notification skipped");
                    return;
                }

                SKSE::log::info("SKSE save-game restoration completed");
                LoadingMenuListener::GetSingleton()->NotifyAfterLoad();
            }
        });

        return true;
    }
}
