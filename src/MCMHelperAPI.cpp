#include "MCMHelperAPI.h"

#include <fstream>
#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <spdlog/spdlog.h>

namespace
{
    class RuntimeVersionResultCallback final : public RE::BSScript::IStackCallbackFunctor
    {
    public:
        explicit RuntimeVersionResultCallback(MCMHelperAPI::RuntimeVersionCallback callback)
            : callback_(std::move(callback))
        {}

        void operator()(RE::BSScript::Variable result) override
        {
            if (!result.IsInt() || result.GetSInt() < 0) {
                callback_(std::nullopt);
                return;
            }

            callback_(static_cast<std::uint32_t>(result.GetSInt()));
        }

        void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

    private:
        MCMHelperAPI::RuntimeVersionCallback callback_;
    };
}

void MCMHelperAPI::GetRuntimeVersion(RuntimeVersionCallback callback)
{
    if (!callback) {
        return;
    }

    const auto* skyrimVM = RE::SkyrimVM::GetSingleton();
    const auto vm = skyrimVM ? skyrimVM->impl : nullptr;
    if (!vm) {
        callback(std::nullopt);
        return;
    }

    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> resultCallback{
        new RuntimeVersionResultCallback(callback)
    };
    std::unique_ptr<RE::BSScript::IFunctionArguments> arguments{RE::MakeFunctionArguments()};

    if (!vm->DispatchStaticCall("MCM", "GetVersionCode", arguments.get(), resultCallback)) {
        callback(std::nullopt);
    }
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

std::string MCMHelperAPI::ReloadMenuMessageText(const std::string_view currentText)
{
    if (!Reload()) {
        spdlog::error("MCM menu text reload failed; retaining displayed text");
        return std::string(currentText);
    }
    auto text = GetModSettingString("sMessageText:General", "");
    spdlog::debug("MCM menu text reloaded from disk ({} bytes)", text.size());
    return text;
}

std::int32_t MCMHelperAPI::ReloadMenuMessageDelay(const std::int32_t currentDelay)
{
    if (!Reload()) {
        spdlog::error("MCM menu delay reload failed; retaining displayed delay");
        return currentDelay;
    }
    const auto delay = std::clamp(GetModSettingInt("iMessageDelay:General", 180), 10, 600);
    spdlog::debug("MCM menu delay reloaded from disk ({} seconds)", delay);
    return delay;
}

bool MCMHelperAPI::SaveMenuMessageDelay(const std::int32_t delay)
{
    std::scoped_lock lock(mutex_);
    CSimpleIniA overrides;
    overrides.SetUnicode();
    if (!LoadIfPresent_(overrides, settingsPath_)) {
        return false;
    }
    overrides.SetLongValue("General", "iMessageDelay", std::clamp(delay, 10, 600));
    std::string contents;
    if (overrides.Save(contents) < SI_OK) {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(settingsPath_.parent_path(), error);
    std::ofstream stream(settingsPath_, std::ios::binary | std::ios::trunc);
    stream << contents;
    stream.close(); // explicit close so we can check for errors
    if (error || !stream) {
        spdlog::error("MCM menu delay could not be saved: {}", settingsPath_.string());
        return false;
    }
    spdlog::info("MCM menu delay saved ({} seconds)", std::clamp(delay, 10, 600));
    return true;
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
        const bool missing = !std::filesystem::exists(path);
        if (missing) {
            spdlog::debug("MCM settings file absent: {}", path.string());
        } else {
            spdlog::error("MCM settings file cannot be opened: {}", path.string());
        }
        return missing;
    }
    const std::string contents(std::istreambuf_iterator<char>(stream), {});
    const auto result = ini.LoadData(contents);
    spdlog::debug("MCM settings read: {} ({} bytes, parse result={})", path.string(), contents.size(), result);
    return result >= SI_OK;
}
