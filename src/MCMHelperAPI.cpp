#include "MCMHelperAPI.h"

#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

std::optional<std::uint32_t> MCMHelperAPI::GetRuntimeVersion(const SKSE::LoadInterface& skse)
{
    const auto* pluginInfo = skse.GetPluginInfo(PluginName.data());
    if (pluginInfo == nullptr) {
        return std::nullopt;
    }
    return pluginInfo->version;
}

MCMHelperAPI::MCMHelperAPI(std::string modName, std::filesystem::path dataDirectory)
    : defaultsPath_(dataDirectory / "MCM" / "Config" / modName / "settings.ini"),
      settingsPath_(dataDirectory / "MCM" / "Settings" / (std::move(modName) + ".ini"))
{
    ini_.SetUnicode();
}

bool MCMHelperAPI::Reload()
{
    std::scoped_lock lock(mutex_);
    ini_.Reset();
    ini_.SetUnicode();

    // MCM Helper uses the config file for defaults and overlays the player's
    // settings file on top of it.
    return LoadIfPresent_(ini_, defaultsPath_) && LoadIfPresent_(ini_, settingsPath_);
}

std::int32_t MCMHelperAPI::GetModSettingInt(const std::string_view settingId,
                                            const std::int32_t defaultValue) const
{
    const auto key = ParseSettingId_(settingId);
    std::scoped_lock lock(mutex_);
    const auto value = ini_.GetLongValue(key.section.c_str(), key.name.c_str(), defaultValue);

    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) {
        return defaultValue;
    }
    return static_cast<std::int32_t>(value);
}

double MCMHelperAPI::GetModSettingFloat(const std::string_view settingId,
                                        const double defaultValue) const
{
    const auto key = ParseSettingId_(settingId);
    std::scoped_lock lock(mutex_);
    return ini_.GetDoubleValue(key.section.c_str(), key.name.c_str(), defaultValue);
}

bool MCMHelperAPI::GetModSettingBool(const std::string_view settingId, const bool defaultValue) const
{
    const auto key = ParseSettingId_(settingId);
    std::scoped_lock lock(mutex_);
    return ini_.GetBoolValue(key.section.c_str(), key.name.c_str(), defaultValue);
}

std::string MCMHelperAPI::GetModSettingString(const std::string_view settingId,
                                              const std::string_view defaultValue) const
{
    const auto key = ParseSettingId_(settingId);
    const std::string fallback(defaultValue);
    std::scoped_lock lock(mutex_);
    return ini_.GetValue(key.section.c_str(), key.name.c_str(), fallback.c_str());
}

MCMHelperAPI::SettingKey MCMHelperAPI::ParseSettingId_(const std::string_view settingId)
{
    const auto separator = settingId.find(':');
    if (separator == std::string_view::npos || separator == 0 || separator + 1 == settingId.size()) {
        throw std::invalid_argument("MCM setting IDs must use the form name:section");
    }

    return {
        .section = std::string(settingId.substr(separator + 1)),
        .name = std::string(settingId.substr(0, separator)),
    };
}

bool MCMHelperAPI::LoadIfPresent_(CSimpleIniA& ini, const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return !std::filesystem::exists(path);
    }
    const std::string contents(std::istreambuf_iterator<char>(stream), {});
    return ini.LoadData(contents) >= SI_OK;
}
