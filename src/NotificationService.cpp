#include "NotificationService.h"

#include <chrono>
#include <string>

using namespace std::chrono_literals;

NotificationService::NotificationService(OpenVRTransportDetector& transportDetector)
    : transportDetector_(transportDetector) {}

NotificationService::~NotificationService() { Stop(); }

void NotificationService::Start() {
    Stop();
    thread_ = std::jthread([this](const std::stop_token& stopToken) {
        std::unique_lock lock(mutex_);

        while (!stopToken.stop_requested()) {
            timer_.wait_for(lock, stopToken, message_delay_, [] { return false; });
            if (stopToken.stop_requested()) {
                break;
            }

            auto message = std::string(GetMessage_(transportDetector_.GetTransport()));
            SKSE::GetTaskInterface()->AddUITask([message = std::move(message)] {
                RE::DebugNotification(message.c_str());
                SKSE::log::info("{}", message);
            });
        }
    });
}

void NotificationService::Stop() {
    thread_.request_stop();
    timer_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

const char* NotificationService::GetMessage_(const OpenVRTransportDetector::Transport transport) noexcept {
    switch (transport) {
        case OpenVRTransportDetector::Transport::kWired:
            return "Zenithar's courier travels through the earth.";
        case OpenVRTransportDetector::Transport::kWireless:
            return "Zenithar's courier travels through the air.";
        default:
            return "Zenithar's courier is confused";
    }
}
