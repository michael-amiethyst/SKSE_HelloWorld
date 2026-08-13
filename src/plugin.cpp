#include <spdlog/sinks/basic_file_sink.h>

#include <openvr/openvr_capi.h>
#include <Windows.h>

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

    enum class HeadsetTransport
    {
        kUnknown,
        kWired,
        kWireless
    };

    VR_IVRSystem_FnTable* GetOpenVRSystem()
    {
        using GetGenericInterface = std::intptr_t(__cdecl*)(const char*, EVRInitError*);

        auto openVR = GetModuleHandleW(L"openvr_api.dll");
        if (!openVR) {
            wchar_t executablePath[MAX_PATH]{};
            const auto pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
            if (pathLength == 0 || pathLength == MAX_PATH) {
                return nullptr;
            }

            const auto dllPath = std::filesystem::path(executablePath).parent_path() / L"openvr_api.dll";
            openVR = LoadLibraryW(dllPath.c_str());
            if (!openVR) {
                return nullptr;
            }
        }

        const auto getGenericInterface = reinterpret_cast<GetGenericInterface>(
            GetProcAddress(openVR, "VR_GetGenericInterface"));
        if (!getGenericInterface) {
            return nullptr;
        }

        const auto interfaceName = std::string("FnTable:") + IVRSystem_Version;
        auto error = EVRInitError_VRInitError_None;
        const auto system = getGenericInterface(interfaceName.c_str(), &error);
        if (error != EVRInitError_VRInitError_None || system == 0) {
            return nullptr;
        }

        return reinterpret_cast<VR_IVRSystem_FnTable*>(system);
    }

    HeadsetTransport GetHeadsetTransport()
    {
        auto* openVRSystem = GetOpenVRSystem();
        if (!openVRSystem) {
            return HeadsetTransport::kUnknown;
        }

        auto error = ETrackedPropertyError_TrackedProp_Success;
        const auto isWireless = openVRSystem->GetBoolTrackedDeviceProperty(
            k_unTrackedDeviceIndex_Hmd,
            ETrackedDeviceProperty_Prop_DeviceIsWireless_Bool,
            &error);
        if (error != ETrackedPropertyError_TrackedProp_Success) {
            return HeadsetTransport::kUnknown;
        }

        return isWireless ? HeadsetTransport::kWireless : HeadsetTransport::kWired;
    }

    const char* GetHeadsetTransportMessage()
    {
        switch (GetHeadsetTransport()) {
        case HeadsetTransport::kWired:
            return "Zenithar's courier travels through the earth.";
        case HeadsetTransport::kWireless:
            return "Zenithar's courier travels through the air.";
        default:
            return "Zenithar's courier is confused";
        }
    }

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
                    const auto* message = GetHeadsetTransportMessage();
                    RE::DebugNotification(message);
                    SKSE::log::info("{}", message);
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

    /** Plugin's main */
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
