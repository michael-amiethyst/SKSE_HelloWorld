#include <catch2/catch_test_macros.hpp>

#include "MessageTextLayout.h"

#include <string>
#include <vector>

using MessageTextLayout::GetPreview;
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

TEST_CASE("Popup previews through 96 characters end normally", "[MessageTextLayout]")
{
    for (const auto length : {0, 1, 95, 96}) {
        const auto preview = GetPreview(std::string(length, 'x'));
        CHECK_FALSE(preview.exceedsPopupLimit);
        CHECK(preview.EndMarker() == "End of Message");
        CHECK(preview.lines.size() <= MessageTextLayout::PreviewLines);
        std::string recovered;
        for (const auto& line : preview.lines) {
            CHECK(line.size() <= MessageTextLayout::RowWidth);
            recovered += line;
        }
        CHECK(recovered == std::string(length, 'x'));
    }
}

TEST_CASE("Popup previews warn above 96 characters and show only the popup prefix", "[MessageTextLayout]")
{
    for (const auto length : {97, 10000}) {
        const std::string original = std::string(96, 'x') + std::string(length - 96, 'z');
        const auto preview = GetPreview(original);
        CHECK(preview.exceedsPopupLimit);
        CHECK(preview.EndMarker() == MessageTextLayout::CutoffWarning);
        CHECK(preview.lines == std::vector<std::string>{std::string(32, 'x'), std::string(32, 'x'), std::string(32, 'x')});
        CHECK(original.size() == static_cast<std::size_t>(length));
    }
}

TEST_CASE("Popup preview preserves four explicit lines", "[MessageTextLayout]")
{
    const auto preview = GetPreview("First\nSecond\nThird\nFourth");
    CHECK(preview.lines == std::vector<std::string>{"First", "Second", "Third", "Fourth"});
    CHECK(preview.EndMarker() == "End of Message");
}

TEST_CASE("Popup preview packs excess short lines without losing content", "[MessageTextLayout]")
{
    const auto preview = GetPreview("1\n2\r3\t\n4\f\v5\n6");
    CHECK(preview.lines == std::vector<std::string>{"1 2 3  4  5 6"});
    CHECK_FALSE(preview.exceedsPopupLimit);
    CHECK(preview.EndMarker() == "End of Message");
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
    CHECK(preview.EndMarker() == "End of Message");
}

TEST_CASE("Popup preview counts UTF-8 characters rather than bytes at the limit", "[MessageTextLayout]")
{
    for (const std::string character : {"\xC3\xA9", "\xE7\x95\x8C", "\xF0\x9F\x98\x80"}) {
        std::string text;
        for (int i = 0; i < 96; ++i) {
            text += character;
        }
        const auto exact = GetPreview(text);
        CHECK_FALSE(exact.exceedsPopupLimit);
        const auto longer = GetPreview(text + character);
        CHECK(longer.exceedsPopupLimit);
        CHECK(longer.lines == exact.lines);
        std::string recovered;
        for (const auto& line : longer.lines) {
            recovered += line;
        }
        CHECK(recovered == text);
    }
}

TEST_CASE("Popup preview marker labels fit a single menu row", "[MessageTextLayout]")
{
    CHECK(MessageTextLayout::EndOfMessage.size() <= MessageTextLayout::RowWidth);
    CHECK(MessageTextLayout::CutoffWarning.size() <= MessageTextLayout::RowWidth);
}
