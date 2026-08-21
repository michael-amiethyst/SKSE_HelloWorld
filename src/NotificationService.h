#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "HeadMountedDisplay.h"

class NotificationService
{
public:
    explicit NotificationService(HeadMountedDisplay& transportDetector);
    ~NotificationService();

    NotificationService(const NotificationService&) = delete;
    NotificationService& operator=(const NotificationService&) = delete;

    void Start();
    void Stop();

    // VisibleForTesting
    static const char* GetMessage_(HeadMountedDisplay::Transport transport) noexcept;

private:
    static constexpr auto message_delay_ = std::chrono::minutes{3};

    HeadMountedDisplay& transportDetector_;
    std::jthread thread_;
    std::condition_variable_any timer_;
    std::mutex mutex_;
};
