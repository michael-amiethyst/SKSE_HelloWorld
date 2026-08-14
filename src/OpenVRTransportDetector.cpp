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
            if (!module) {
                wchar_t executablePath[MAX_PATH]{};
                const auto pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
                if (pathLength == 0 || pathLength == MAX_PATH) {
                    return nullptr;
                }

                const auto dllPath = std::filesystem::path(executablePath).parent_path() / L"openvr_api.dll";
                module = LoadLibraryW(dllPath.c_str());
                loadedModule = module != nullptr;
            }
        }

        if (!module) {
            return nullptr;
        }

        const auto getGenericInterface = reinterpret_cast<GetGenericInterface>(
            GetProcAddress(module, "VR_GetGenericInterface"));
        if (!getGenericInterface) {
            return nullptr;
        }

        const auto interfaceName = std::string("FnTable:") + IVRSystem_Version;
        auto error = EVRInitError_VRInitError_None;
        const auto interfacePointer = getGenericInterface(interfaceName.c_str(), &error);
        if (error != EVRInitError_VRInitError_None || interfacePointer == 0) {
            return nullptr;
        }

        system = reinterpret_cast<VR_IVRSystem_FnTable*>(interfacePointer);
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

OpenVRTransportDetector::Transport OpenVRTransportDetector::GetTransport()
{
    auto* system = impl_->GetSystem();
    if (!system) {
        return Transport::kUnknown;
    }

    auto detailsEnum = ETrackedPropertyError_TrackedProp_Success;
    const auto isWireless = system->GetBoolTrackedDeviceProperty(
        k_unTrackedDeviceIndex_Hmd, ETrackedDeviceProperty_Prop_DeviceIsWireless_Bool, &detailsEnum);
    return DetermineTransport_(detailsEnum == ETrackedPropertyError_TrackedProp_Success, isWireless);
}

OpenVRTransportDetector::Transport OpenVRTransportDetector::DetermineTransport_(
    bool propertyReadSucceeded, bool isWireless) noexcept
{
    if (!propertyReadSucceeded) {
        return Transport::kUnknown;
    }
    return isWireless ? Transport::kWireless : Transport::kWired;
}
