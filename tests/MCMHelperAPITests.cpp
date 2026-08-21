#include <catch2/catch_test_macros.hpp>

#include "MCMHelperAPI.h"

#include <chrono>
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

TEST_CASE("MCM Helper API overlays player settings on config defaults")
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

TEST_CASE("MCM Helper API returns caller defaults when settings files are absent")
{
    const TestDataDirectory data;
    MCMHelperAPI api("HelloWorld", data.Get());
    REQUIRE(api.Reload());

    CHECK(api.GetModSettingInt("iMessageDelay:General", 180) == 180);
    CHECK(api.GetModSettingString("sLabel:General", "fallback") == "fallback");
}

TEST_CASE("MCM Helper API rejects malformed setting IDs")
{
    const TestDataDirectory data;
    MCMHelperAPI api("HelloWorld", data.Get());

    CHECK_THROWS_AS(api.GetModSettingInt("iMessageDelay", 180), std::invalid_argument);
}
