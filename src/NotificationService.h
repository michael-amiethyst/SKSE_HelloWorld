#pragma once

#include "OpenVRTransportDetector.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class NotificationService
{
public:
    explicit NotificationService(OpenVRTransportDetector& transportDetector);
    ~NotificationService();

    NotificationService(const NotificationService&) = delete;
    NotificationService& operator=(const NotificationService&) = delete;

    void Start();
    void Stop();

    // VisibleForTesting
    static const char* GetMessage_(OpenVRTransportDetector::Transport transport) noexcept;

private:
    static constexpr auto message_delay_ = std::chrono::seconds{60};

    OpenVRTransportDetector& transportDetector_;
    std::jthread thread_;
    std::condition_variable_any timer_;
    std::mutex mutex_;
};
