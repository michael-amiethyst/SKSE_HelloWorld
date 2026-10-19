#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace MessageTextLayout
{
    // Leave a character of margin below the observed 33-character MCM cutoff.
    inline constexpr std::size_t RowWidth = 32;
    inline constexpr std::size_t PreviewLines = 4;
    inline constexpr std::size_t PopupCharacterLimit = 96;
    inline constexpr std::string_view TruncationSuffix = " [truncated]";
    inline constexpr std::size_t MenuCharacterLimit = PopupCharacterLimit + TruncationSuffix.size();
    static_assert(MenuCharacterLimit <= RowWidth * PreviewLines);

    struct Preview
    {
        std::vector<std::string> lines;
    };

    [[nodiscard]] std::vector<std::string> Wrap(std::string_view text);
    // Popups show the first 96 characters. Only the menu appends the suffix.
    [[nodiscard]] std::string LimitPopupMessage(std::string_view text);
    [[nodiscard]] std::string FormatMenuMessage(std::string_view text);
    [[nodiscard]] Preview GetPreview(std::string_view text);
}
