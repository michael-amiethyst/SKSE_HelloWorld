#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace MessageTextLayout
{
    // Leave a character of margin below the observed 33-character MCM cutoff.
    inline constexpr std::size_t RowWidth = 32;
    inline constexpr std::size_t RowsPerPage = 8;

    struct Page
    {
        std::vector<std::string> lines;
        std::size_t index;
        std::size_t count;
    };

    [[nodiscard]] std::vector<std::string> Wrap(std::string_view text);
    [[nodiscard]] Page GetPage(std::string_view text, int requestedPage);
}
