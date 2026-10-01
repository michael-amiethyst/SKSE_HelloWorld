#include "NotificationService.h"

#include <chrono>
#include <string>
#include <utility>

NotificationService::NotificationService(TransportProvider transportProvider,
                                         NotificationSender notificationSender)
    : transport_provider_(std::move(transportProvider)),
      notification_sender_(std::move(notificationSender)) {}

NotificationService::~NotificationService() { Stop(); }

void NotificationService::Start() {
    Stop();
    thread_ = std::jthread([this](const std::stop_token& stopToken) {
        std::unique_lock lock(mutex_);

        while (!stopToken.stop_requested()) {
            const auto generation = delay_generation_;
            const auto delay = message_delay_;
            const bool delayChanged = timer_.wait_for(lock, stopToken, delay, [this, generation] {
                return delay_generation_ != generation;
            });
            if (stopToken.stop_requested()) {
                break;
            }
            if (delayChanged) {
                continue;
            }

            notification_sender_(std::string(GetMessage_(transport_provider_())));
        }
    });
}

void NotificationService::SetMessageDelay(const std::chrono::steady_clock::duration delay) {
    {
        std::scoped_lock lock(mutex_);
        message_delay_ = delay;
        ++delay_generation_;
    }
    timer_.notify_all();
}

void NotificationService::Stop() {
    thread_.request_stop();
    timer_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

const char* NotificationService::GetMessage_(const HeadMountedDisplay::Transport transport) noexcept {
    switch (transport) {
        case HeadMountedDisplay::Transport::kWired:
            return "Zenithar's courier travels through the earth.";
        case HeadMountedDisplay::Transport::kWireless:
            return "Zenithar's courier travels through the air.";
        default:
            return "Zenithar's courier is confused";
    }
}
