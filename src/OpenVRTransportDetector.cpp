#include "OpenVRTransportDetector.h"

#include <openvr/openvr_capi.h>
#include <Windows.h>

#include <filesystem>
#include <string>

struct OpenVRTransportDetector::Impl
{
    using GetGenericInterface = std::intptr_t(__cdecl*)(const char*, EVRInitError*);

    ~Impl()
    {
        if (loadedModule) {
            FreeLibrary(module);
        }
    }

    VR_IVRSystem_FnTable* GetSystem()
    {
        if (system) {
            return system;
        }

        if (!module) {
            module = GetModuleHandleW(L"openvr_api.dll");
            if (module) {
                SKSE::log::debug("OpenVR transport detection: found openvr_api.dll already loaded");
            } else {
                SKSE::log::debug(
                    "OpenVR transport detection: openvr_api.dll is not loaded; trying the game directory");
                wchar_t executablePath[MAX_PATH]{};
                if (const auto pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
                    pathLength == 0 || pathLength == MAX_PATH) {
                    SKSE::log::error(
                        "OpenVR transport detection failed: GetModuleFileNameW returned length {} "
                        "(Win32 error {})",
                        pathLength, GetLastError());
                    return nullptr;
                }

                const auto dllPath = std::filesystem::path(executablePath).parent_path() / L"openvr_api.dll";
                module = LoadLibraryW(dllPath.c_str());
                loadedModule = module != nullptr;
                if (module) {
                    SKSE::log::debug("OpenVR transport detection: loaded {}", dllPath.string());
                } else {
                    SKSE::log::error(
                        "OpenVR transport detection failed: could not load {} (Win32 error {})",
                        dllPath.string(), GetLastError());
                    return nullptr;
                }
            }
        }

        const auto getGenericInterface = reinterpret_cast<GetGenericInterface>(
            GetProcAddress(module, "VR_GetGenericInterface"));
        if (!getGenericInterface) {
            SKSE::log::error(
                "OpenVR transport detection failed: VR_GetGenericInterface was not exported "
                "(Win32 error {})",
                GetLastError());
            return nullptr;
        }

        const auto interfaceName = std::string("FnTable:") + IVRSystem_Version;
        SKSE::log::debug("OpenVR transport detection: requesting interface {}", interfaceName);
        auto error = EVRInitError_VRInitError_None;
        const auto interfacePointer = getGenericInterface(interfaceName.c_str(), &error);
        if (error != EVRInitError_VRInitError_None || interfacePointer == 0) {
            SKSE::log::error(
                "OpenVR transport detection failed: VR_GetGenericInterface returned error {} and pointer {}",
                static_cast<int>(error), interfacePointer);
            return nullptr;
        }

        system = reinterpret_cast<VR_IVRSystem_FnTable*>(interfacePointer);
        SKSE::log::debug("OpenVR transport detection: acquired IVRSystem interface");
        return system;
    }

    HMODULE module{};
    VR_IVRSystem_FnTable* system{};
    bool loadedModule{};
};

OpenVRTransportDetector::OpenVRTransportDetector() :
    impl_(std::make_unique<Impl>())
{}

OpenVRTransportDetector::~OpenVRTransportDetector() = default;

OpenVRTransportDetector::Transport OpenVRTransportDetector::GetTransport() const
{
    SKSE::log::debug("OpenVR transport detection started");
    const auto* system = impl_->GetSystem();
    if (!system) {
        SKSE::log::warn("OpenVR transport detection result: unknown (IVRSystem unavailable)");
        return Transport::kUnknown;
    }

    auto detailsEnum = ETrackedPropertyError_TrackedProp_Success;
    const auto isWireless = system->GetBoolTrackedDeviceProperty(
        k_unTrackedDeviceIndex_Hmd, ETrackedDeviceProperty_Prop_DeviceIsWireless_Bool, &detailsEnum);
    if (detailsEnum == ETrackedPropertyError_TrackedProp_UnknownProperty) {
        // under Meta Horizon Link app DeviceIsWireless isn't even exposed
        SKSE::log::warn("DeviceIsWireless property was not found, assuming wired connection");
        return Transport::kWired;
    } else if (detailsEnum != ETrackedPropertyError_TrackedProp_Success) {
        SKSE::log::error(
            "OpenVR transport detection failed: Prop_DeviceIsWireless_Bool for HMD returned property error {}",
            static_cast<int>(detailsEnum));
        return Transport::kUnknown;
    }

    const auto transport = DetermineTransport_(true, isWireless);
    SKSE::log::info("OpenVR transport detection result: {}", isWireless ? "wireless" : "wired");
    return transport;
}

OpenVRTransportDetector::Transport OpenVRTransportDetector::DetermineTransport_(const bool propertyReadSucceeded,
                                                                                const bool isWireless) noexcept
{
    if (!propertyReadSucceeded) {
        return Transport::kUnknown;
    }
    return isWireless ? Transport::kWireless : Transport::kWired;
}
