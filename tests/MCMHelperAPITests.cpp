#include <catch2/catch_test_macros.hpp>

#include "MCMHelperAPI.h"
#include "MessageTextLayout.h"

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace
{
    class TestDataDirectory
    {
    public:
        TestDataDirectory()
            : path_(std::filesystem::temp_directory_path() /
                    ("HelloWorld-MCMHelperAPI-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
        {
            std::filesystem::create_directories(path_ / "MCM" / "Config" / "HelloWorld");
            std::filesystem::create_directories(path_ / "MCM" / "Settings");
        }

        ~TestDataDirectory() { std::filesystem::remove_all(path_); }

        [[nodiscard]] const std::filesystem::path& Get() const { return path_; }

    private:
        std::filesystem::path path_;
    };

    void WriteFile(const std::filesystem::path& path, const std::string_view contents)
    {
        std::ofstream stream(path, std::ios::binary);
        stream << contents;
    }
}

TEST_CASE("MCM Helper API overlays player settings on config defaults", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    WriteFile(data.Get() / "MCM" / "Config" / "HelloWorld" / "settings.ini",
              "[General]\niMessageDelay=180\nfScale=1.5\nbEnabled=true\nsLabel=default\n");
    WriteFile(data.Get() / "MCM" / "Settings" / "HelloWorld.ini",
              "[General]\niMessageDelay=45\nsLabel=player\n");

    MCMHelperAPI api("HelloWorld", data.Get());
    REQUIRE(api.Reload());

    CHECK(api.GetModSettingInt("iMessageDelay:General", 0) == 45);
    CHECK(api.GetModSettingFloat("fScale:General", 0.0) == 1.5);
    CHECK(api.GetModSettingBool("bEnabled:General", false));
    CHECK(api.GetModSettingString("sLabel:General", "missing") == "player");
}

TEST_CASE("MCM Helper API returns caller defaults when settings files are absent", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    MCMHelperAPI api("HelloWorld", data.Get());
    REQUIRE(api.Reload());

    CHECK(api.GetModSettingInt("iMessageDelay:General", 180) == 180);
    CHECK(api.GetModSettingString("sLabel:General", "fallback") == "fallback");
}

TEST_CASE("MCM Helper API rejects malformed setting IDs", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    MCMHelperAPI api("HelloWorld", data.Get());

    CHECK_THROWS_AS(api.GetModSettingInt("iMessageDelay", 180), std::invalid_argument);
}

TEST_CASE("Menu text reload reads external edits without rewriting the player INI", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    const auto defaults = data.Get() / "MCM/Config/HelloWorld/settings.ini";
    const auto overrides = data.Get() / "MCM/Settings/HelloWorld.ini";
    WriteFile(defaults, "[General]\nsMessageText=default\n");
    MCMHelperAPI api("HelloWorld", data.Get());
    std::string displayed = api.ReloadMenuMessageText("old cached text");
    CHECK(displayed == "default");
    for (const std::string text : {"Edited while open", "Second edit", "", "  "}) {
        const auto contents = "[General]\nsMessageText=" + text + "\niMessageDelay=600\n";
        WriteFile(overrides, contents);
        // Closing and reopening the menu calls this same native reader.
        displayed = api.ReloadMenuMessageText(displayed);
        CHECK(displayed == (text == "  " ? "" : text)); // SimpleIni trims INI whitespace.
        std::ifstream stream(overrides, std::ios::binary);
        CHECK(std::string(std::istreambuf_iterator<char>(stream), {}) == contents);
    }
    std::filesystem::remove(overrides);
    CHECK(api.ReloadMenuMessageText(displayed) == "default");
    std::filesystem::remove(defaults);
    CHECK(api.ReloadMenuMessageText("previous").empty());
}

TEST_CASE("Popup INI reads limit overlength values and accept exactly 86 characters", "[MCMHelperAPI][MessageTextLayout][integration]")
{
    const TestDataDirectory data;
    const auto overrides = data.Get() / "MCM/Settings/HelloWorld.ini";
    MCMHelperAPI api("HelloWorld", data.Get());
    for (const auto length : {87, 86, 96, 0}) {
        const auto contents = "[General]\nsMessageText=" + std::string(length, 'x') + '\n';
        WriteFile(overrides, contents);
        const auto text = api.ReloadMenuMessageText("stale");
        const auto expected = length > 86 ? std::string(86, 'x') + " [truncated]" : std::string(length, 'x');
        CHECK(text == expected);
        CHECK(text.size() <= 98);
        CHECK(MessageTextLayout::GetPreview(text).lines.size() <= 4);
        std::ifstream stream(overrides, std::ios::binary);
        CHECK(std::string(std::istreambuf_iterator<char>(stream), {}) == contents);
    }
}

TEST_CASE("INI text truncates at character 87 before menu word wrapping", "[MCMHelperAPI][MessageTextLayout][integration]")
{
    const TestDataDirectory data;
    const auto defaults = data.Get() / "MCM/Config/HelloWorld/settings.ini";
    const auto overrides = data.Get() / "MCM/Settings/HelloWorld.ini";
    const std::string group = "12345678 12345678 12345678";
    const auto prefix = group + ' ' + group + ' ' + group + " END86";
    REQUIRE(prefix.size() == 86);
    MCMHelperAPI api("HelloWorld", data.Get());

    // Exercise both config defaults and player overrides through the same reader
    // and preview calls used by the Papyrus menu. Spaces count toward the limit.
    for (const auto& path : {defaults, overrides}) {
        for (const std::size_t length : {85, 86, 87, 96, 98, 128, 129, 256}) {
            CAPTURE(path, length);
            const auto original = length <= 86 ? prefix.substr(0, length) : prefix + std::string(length - 86, 'Z');
            const auto contents = "[General]\nsMessageText=" + original + '\n';
            WriteFile(path, contents);
            const auto expected = length > 86 ? prefix + " [truncated]" : original;
            const auto menu = api.ReloadMenuMessageText("stale menu value");
            CHECK(menu == expected);
            auto expectedRows = std::vector<std::string>{group, group, group + (length == 85 ? " END8" : " END86")};
            if (length > 86) {
                expectedRows.emplace_back("[truncated]");
            }
            CHECK(MessageTextLayout::GetPreview(menu).lines == expectedRows);
            CHECK(MessageTextLayout::LimitPopupMessage(api.GetModSettingString("sMessageText:General", "")) ==
                  original.substr(0, 86));
            std::ifstream stream(path, std::ios::binary);
            CHECK(std::string(std::istreambuf_iterator<char>(stream), {}) == contents);
        }
    }
}

TEST_CASE("Menu delay reload follows external edits and clamps to the notification range", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    const auto defaults = data.Get() / "MCM/Config/HelloWorld/settings.ini";
    const auto overrides = data.Get() / "MCM/Settings/HelloWorld.ini";
    WriteFile(defaults, "[General]\niMessageDelay=180\n");
    MCMHelperAPI api("HelloWorld", data.Get());
    CHECK(api.ReloadMenuMessageDelay(10) == 180);
    for (const auto delay : {10, 600, 120, 0, 900}) {
        const auto contents = "[General]\niMessageDelay=" + std::to_string(delay) + "\nsMessageText=keep me\n";
        WriteFile(overrides, contents);
        CHECK(api.ReloadMenuMessageDelay(180) == std::clamp(delay, 10, 600));
        std::ifstream stream(overrides, std::ios::binary);
        CHECK(std::string(std::istreambuf_iterator<char>(stream), {}) == contents);
    }
    WriteFile(overrides, "[General]\niMessageDelay=invalid\n");
    CHECK(api.ReloadMenuMessageDelay(10) == 180);
    std::filesystem::remove(overrides);
    CHECK(api.ReloadMenuMessageDelay(10) == 180);
    std::filesystem::remove(defaults);
    CHECK(api.ReloadMenuMessageDelay(10) == 180);
}

TEST_CASE("Menu slider saves after an external edit even when selecting the old cached delay", "[MCMHelperAPI]")
{
    const TestDataDirectory data;
    const auto overrides = data.Get() / "MCM/Settings/HelloWorld.ini";
    WriteFile(overrides, "[General]\niMessageDelay=10\nsMessageText=first\n");
    MCMHelperAPI api("HelloWorld", data.Get());
    REQUIRE(api.Reload());
    WriteFile(overrides, "[General]\niMessageDelay=600\nsMessageText=external edit\n[Other]\niValue=42\n");
    REQUIRE(api.SaveMenuMessageDelay(10));
    CHECK(api.ReloadMenuMessageDelay(180) == 10);
    CHECK(api.GetModSettingString("sMessageText:General", "") == "external edit");
    CHECK(api.GetModSettingInt("iValue:Other", 0) == 42);
    std::filesystem::remove(overrides);
    REQUIRE(api.SaveMenuMessageDelay(900));
    CHECK(api.ReloadMenuMessageDelay(180) == 600);
    REQUIRE(api.SaveMenuMessageDelay(0));
    CHECK(api.ReloadMenuMessageDelay(180) == 10);
}
