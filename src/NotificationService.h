#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "HeadMountedDisplay.h"

class MCMHelperAPI;

class NotificationService
{
public:
    using TransportProvider = std::function<HeadMountedDisplay::Transport()>;
    using NotificationSender = std::function<void(std::string)>;

    NotificationService(TransportProvider transportProvider, NotificationSender notificationSender);
    ~NotificationService();

    NotificationService(const NotificationService&) = delete;
    NotificationService& operator=(const NotificationService&) = delete;

    void Start();
    void Stop();
    // Restart an active countdown with the new interval.
    void SetMessageDelay(std::chrono::steady_clock::duration delay);
    // Blank text restores transport messages; preserve nonblank text verbatim.
    void SetMessageText(std::string text);
    // Reload MCM defaults/overrides, retaining active settings if loading fails.
    bool ReloadSettings(MCMHelperAPI& settings);

    // VisibleForTesting
    static const char* GetMessage_(HeadMountedDisplay::Transport transport) noexcept;

private:
    std::chrono::steady_clock::duration message_delay_{std::chrono::minutes{3}};
    std::uint64_t delay_generation_{0};
    std::string message_text_;

    TransportProvider transport_provider_;
    NotificationSender notification_sender_;
    std::jthread thread_;
    std::condition_variable_any timer_;
    std::mutex mutex_;
};
