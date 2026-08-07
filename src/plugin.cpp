SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    // Once all plugins and mods are loaded, then the ~ console is ready and can
    // be printed to
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            RE::ConsoleLog::GetSingleton()->Print("SKSE Hello World loaded!");
            // kDataLoaded is delivered while the main-menu UI is being set up.
            // Queue the notification so it runs on the UI thread after setup.
            SKSE::GetTaskInterface()->AddUITask([] {
                RE::DebugNotification("Hello, world!");
            });
        }
    });

    return true;
}
