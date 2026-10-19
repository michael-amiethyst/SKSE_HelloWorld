#include <catch2/catch_test_macros.hpp>

#include "MessageTextLayout.h"

#include <string>
#include <vector>

using MessageTextLayout::GetPreview;
using MessageTextLayout::LimitPopupMessage;
using MessageTextLayout::FormatMenuMessage;
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

TEST_CASE("Popup messages shorter than 96 characters pass unchanged", "[MessageTextLayout]")
{
    for (const auto length : {0, 1, 95}) {
        CHECK(LimitPopupMessage(std::string(length, 'x')) == std::string(length, 'x'));
        const auto preview = GetPreview(std::string(length, 'x'));
        CHECK(preview.lines.size() <= MessageTextLayout::PreviewLines);
        std::string recovered;
        for (const auto& line : preview.lines) {
            CHECK(line.size() <= MessageTextLayout::RowWidth);
            recovered += line;
        }
        CHECK(recovered == std::string(length, 'x'));
    }
}

TEST_CASE("A 96-character popup message passes unchanged as valid", "[MessageTextLayout]")
{
    const std::string valid(96, 'x');
    CHECK(LimitPopupMessage(valid) == valid);
    CHECK(FormatMenuMessage(valid) == valid);
    CHECK(LimitPopupMessage(valid).find(" [truncated]") == std::string::npos);
    CHECK(GetPreview(valid).lines == std::vector<std::string>{std::string(32, 'x'), std::string(32, 'x'), std::string(32, 'x')});
}

TEST_CASE("Menu messages retain all 96 popup characters before the truncation suffix", "[MessageTextLayout]")
{
    for (const auto length : {97, 10000}) {
        const std::string original = std::string(96, 'x') + std::string(length - 96, 'z');
        const auto limited = LimitPopupMessage(original);
        CHECK(limited == std::string(96, 'x'));
        CHECK(limited.size() == 96);
        CHECK(LimitPopupMessage(limited) == limited);
        const auto menu = FormatMenuMessage(original);
        CHECK(menu == std::string(96, 'x') + " [truncated]");
        CHECK(menu.size() == 108);
        CHECK(FormatMenuMessage(menu) == menu);
        const auto preview = GetPreview(original);
        CHECK(preview.lines == std::vector<std::string>{std::string(32, 'x'), std::string(32, 'x'), std::string(32, 'x'), "[truncated]"});
        CHECK(original.size() == static_cast<std::size_t>(length));
    }
}

TEST_CASE("Popup preview preserves four explicit lines", "[MessageTextLayout]")
{
    const auto preview = GetPreview("First\nSecond\nThird\nFourth");
    CHECK(preview.lines == std::vector<std::string>{"First", "Second", "Third", "Fourth"});
}

TEST_CASE("Popup preview packs excess short lines without losing content", "[MessageTextLayout]")
{
    const auto preview = GetPreview("1\n2\r3\t\n4\f\v5\n6");
    CHECK(preview.lines == std::vector<std::string>{"1 2 3  4  5 6"});
}

TEST_CASE("Popup preview packs words that would otherwise require more than four rows", "[MessageTextLayout]")
{
    const auto word = std::string(17, 'x');
    const auto text = word + ' ' + word + ' ' + word + ' ' + word + ' ' + word;
    REQUIRE(Wrap(text).size() == 5);
    const auto preview = GetPreview(text);
    CHECK(preview.lines.size() <= MessageTextLayout::PreviewLines);
    std::string recovered;
    for (const auto& line : preview.lines) {
        CHECK(line.size() <= MessageTextLayout::RowWidth);
        recovered += line;
    }
    CHECK(recovered == text);
}

TEST_CASE("Popup preview counts UTF-8 characters rather than bytes at the limit", "[MessageTextLayout]")
{
    for (const std::string character : {"\xC3\xA9", "\xE7\x95\x8C", "\xF0\x9F\x98\x80"}) {
        std::string text;
        for (int i = 0; i < 96; ++i) {
            text += character;
        }
        CHECK(LimitPopupMessage(text) == text);
        const auto longer = GetPreview(text + character);
        std::string recovered;
        for (const auto& line : longer.lines) {
            recovered += line;
        }
        CHECK(recovered == text + "[truncated]");
        CHECK(FormatMenuMessage(text + character) == text + " [truncated]");
        CHECK(LimitPopupMessage(text + character) == text);
    }
}

TEST_CASE("Full popup prefix and suffix fit the four-row menu budget", "[MessageTextLayout]")
{
    CHECK(MessageTextLayout::MenuCharacterLimit == 108);
    CHECK(MessageTextLayout::MenuCharacterLimit <= MessageTextLayout::RowWidth * MessageTextLayout::PreviewLines);
    const auto word = std::string(17, 'x');
    const auto text = word + ' ' + word + ' ' + word + ' ' + word + ' ' + word + ' ' + word;
    const auto preview = GetPreview(text);
    CHECK(preview.lines.size() <= 4);
    std::string recovered;
    for (const auto& line : preview.lines) {
        CHECK(line.size() <= 32);
        recovered += line;
    }
    CHECK(recovered == text.substr(0, 96) + " [truncated]");
}
