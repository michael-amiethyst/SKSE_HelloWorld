#pragma once

#include <SimpleIni.h>

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>

class MCMHelperAPI
{
public:
    explicit MCMHelperAPI(std::string modName, std::filesystem::path dataDirectory = "Data");

    // Reloads defaults and user overrides from MCM Helper's INI files.
    bool Reload();

    [[nodiscard]] std::int32_t GetModSettingInt(std::string_view settingId,
                                                std::int32_t defaultValue) const;
    [[nodiscard]] double GetModSettingFloat(std::string_view settingId, double defaultValue) const;
    [[nodiscard]] bool GetModSettingBool(std::string_view settingId, bool defaultValue) const;
    [[nodiscard]] std::string GetModSettingString(std::string_view settingId,
                                                  std::string_view defaultValue) const;

private:
    struct SettingKey
    {
        std::string section;
        std::string name;
    };

    [[nodiscard]] static SettingKey ParseSettingId_(std::string_view settingId);
    static bool LoadIfPresent_(CSimpleIniA& ini, const std::filesystem::path& path);

    std::filesystem::path defaultsPath_;
    std::filesystem::path settingsPath_;
    mutable std::mutex mutex_;
    CSimpleIniA ini_;
};
