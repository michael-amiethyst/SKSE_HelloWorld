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
    inline constexpr std::string_view EndOfMessage = "End of Message";
    inline constexpr std::string_view CutoffWarning = "Warning: popup cutoff at 96";

    struct Preview
    {
        std::vector<std::string> lines;
        bool exceedsPopupLimit;

        [[nodiscard]] std::string_view EndMarker() const noexcept
        {
            return exceedsPopupLimit ? CutoffWarning : EndOfMessage;
        }
    };

    [[nodiscard]] std::vector<std::string> Wrap(std::string_view text);
    [[nodiscard]] Preview GetPreview(std::string_view text);
}
