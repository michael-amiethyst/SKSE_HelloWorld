#include <catch2/catch_test_macros.hpp>

#include "MessageTextLayout.h"

#include <limits>
#include <string>
#include <vector>

using MessageTextLayout::GetPage;
using MessageTextLayout::Wrap;

TEST_CASE("Message wrapping leaves short text intact and handles the cutoff boundary", "[MessageTextLayout]")
{
    CHECK(Wrap("A short message") == std::vector<std::string>{"A short message"});
    CHECK(Wrap(std::string(32, 'a')) == std::vector<std::string>{std::string(32, 'a')});
    CHECK(Wrap(std::string(33, 'a')) == std::vector<std::string>{std::string(32, 'a'), "a"});
    CHECK(Wrap(std::string(64, 'a')) == std::vector<std::string>{std::string(32, 'a'), std::string(32, 'a')});
}

TEST_CASE("Message wrapping prefers word boundaries and preserves punctuation and case", "[MessageTextLayout]")
{
    CHECK(Wrap("This is a longer message that must wrap cleanly.") ==
          std::vector<std::string>{"This is a longer message that", "must wrap cleanly."});
    CHECK(Wrap(std::string(30, 'a') + " b") == std::vector<std::string>{std::string(30, 'a') + " b"});
    CHECK(Wrap(std::string(31, 'a') + " b") == std::vector<std::string>{std::string(31, 'a'), "b"});
    CHECK(Wrap("Start " + std::string(70, 'x') + " End") ==
          std::vector<std::string>{"Start", std::string(32, 'x'), std::string(32, 'x'), "xxxxxx End"});
}

TEST_CASE("Message wrapping handles blank text and normalizes horizontal spacing", "[MessageTextLayout]")
{
    CHECK(Wrap("") == std::vector<std::string>{""});
    CHECK(Wrap(" \t\f\v  ") == std::vector<std::string>{""});
    CHECK(Wrap("  First\t second  third \f\v ") == std::vector<std::string>{"First second third"});
}

TEST_CASE("Message wrapping preserves explicit line breaks and empty lines", "[MessageTextLayout]")
{
    CHECK(Wrap("First\r\nSecond\n\nFourth\rFifth\n") ==
          std::vector<std::string>{"First", "Second", "", "Fourth", "Fifth", ""});
    CHECK(Wrap("\n") == std::vector<std::string>{"", ""});
}

TEST_CASE("Message wrapping never splits a UTF-8 character", "[MessageTextLayout]")
{
    const std::string accented = "\xC3\xA9";
    const std::string wide = "\xE7\x95\x8C";
    const std::string emoji = "\xF0\x9F\x98\x80";
    for (const auto& character : {accented, wide, emoji}) {
        const auto first = std::string(31, 'a') + character;
        CHECK(Wrap(first + "z") == std::vector<std::string>{first, "z"});
    }
}

TEST_CASE("Message wrapping tolerates incomplete or malformed UTF-8 without dropping bytes", "[MessageTextLayout]")
{
    const auto incomplete = std::string(32, 'a') + "\xF0\x9F";
    CHECK(Wrap(incomplete) == std::vector<std::string>{std::string(32, 'a'), "\xF0\x9F"});
    CHECK(Wrap("\xC3" "broken") == std::vector<std::string>{"\xC3" "broken"});
}

TEST_CASE("Message pages stay within the MCM row budget and clamp navigation", "[MessageTextLayout]")
{
    const std::string text(32 * 8 + 1, 'x');
    const auto first = GetPage(text, 0);
    REQUIRE(first.lines.size() == MessageTextLayout::RowsPerPage);
    CHECK(first.index == 0);
    CHECK(first.count == 2);
    const auto last = GetPage(text, 1);
    CHECK(last.index == 1);
    CHECK(last.count == 2);
    CHECK(last.lines == std::vector<std::string>{"x"});
    CHECK(GetPage(text, -1).lines == first.lines);
    CHECK(GetPage(text, std::numeric_limits<int>::max()).lines == last.lines);
    CHECK(GetPage("", 99).count == 1);
    CHECK(GetPage(std::string(32 * 8, 'x'), 0).count == 1);
}

TEST_CASE("Message pages select distinct first middle and partial last ranges", "[MessageTextLayout]")
{
    std::string text;
    std::vector<std::string> expected;
    for (int line = 0; line < 20; ++line) {
        expected.push_back("Line " + std::to_string(line));
        if (line > 0) {
            text += '\n';
        }
        text += expected.back();
    }
    std::vector<std::string> recovered;
    for (int index = 0; index < 3; ++index) {
        const auto page = GetPage(text, index);
        CHECK(page.index == static_cast<std::size_t>(index));
        CHECK(page.count == 3);
        CHECK(page.lines.size() == (index == 2 ? 4 : 8));
        recovered.insert(recovered.end(), page.lines.begin(), page.lines.end());
    }
    CHECK(recovered == expected);
}

TEST_CASE("Very long messages remain readable through all pages without truncation", "[MessageTextLayout]")
{
    const std::string original(10000, 'x');
    std::string recovered;
    const auto pages = GetPage(original, 0).count;
    for (std::size_t page = 0; page < pages; ++page) {
        const auto layout = GetPage(original, static_cast<int>(page));
        CHECK(layout.lines.size() <= MessageTextLayout::RowsPerPage);
        for (const auto& line : layout.lines) {
            CHECK(line.size() <= MessageTextLayout::RowWidth);
            recovered += line;
        }
    }
    CHECK(recovered == original);
}
