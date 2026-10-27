#include "MessageTextLayout.h"

#include <utf8.h>

#include <iterator>
#include <utility>

namespace
{
    // Advance by a complete UTF-8 code point, never cutting a multibyte character
    // between rows. Treat malformed bytes as individual characters.
    std::size_t CharacterBytes(const std::string_view text)
    {
        auto next = text.begin();
        try {
            utf8::next(next, text.end());
            return static_cast<std::size_t>(std::distance(text.begin(), next));
        } catch (const utf8::exception&) {
            return 1;
        }
    }

    bool IsSpace(const char value)
    {
        return value == ' ' || value == '\t' || value == '\f' || value == '\v';
    }

    std::size_t PrefixBytes(const std::string_view text, const std::size_t characters)
    {
        std::size_t offset = 0;
        for (std::size_t count = 0; count < characters && offset < text.size(); ++count) {
            offset += CharacterBytes(text.substr(offset));
        }
        return offset;
    }
}

std::vector<std::string> MessageTextLayout::Wrap(const std::string_view text)
{
    std::vector<std::string> lines;
    std::string line;
    std::size_t columns = 0;
    const auto finishLine = [&] {
        lines.push_back(std::move(line));
        line.clear();
        columns = 0;
    };

    for (std::size_t offset = 0; offset < text.size();) {
        if (text[offset] == '\n' || text[offset] == '\r') {
            if (text[offset++] == '\r' && offset < text.size() && text[offset] == '\n') {
                ++offset;
            }
            finishLine();
            continue;
        }
        if (IsSpace(text[offset])) {
            ++offset;
            continue;
        }

        const auto wordStart = offset;
        std::size_t wordColumns = 0;
        while (offset < text.size() && text[offset] != '\n' && text[offset] != '\r' && !IsSpace(text[offset])) {
            offset += CharacterBytes(text.substr(offset));
            ++wordColumns;
        }
        auto word = text.substr(wordStart, offset - wordStart);
        if (columns && columns + 1 + wordColumns > RowWidth) {
            finishLine();
        }
        if (columns) {
            line += ' ';
            ++columns;
        }
        while (!word.empty()) {
            if (columns == RowWidth) {
                finishLine();
            }
            const auto bytes = CharacterBytes(word);
            line.append(word.substr(0, bytes));
            word.remove_prefix(bytes);
            ++columns;
        }
    }
    if (!line.empty() || lines.empty() || text.back() == '\n' || text.back() == '\r') {
        finishLine();
    }
    return lines;
}

std::string MessageTextLayout::LimitPopupMessage(const std::string_view text)
{
    return std::string(text.substr(0, PrefixBytes(text, PopupCharacterLimit)));
}

std::string MessageTextLayout::FormatMenuMessage(const std::string_view text)
{
    auto message = LimitPopupMessage(text);
    if (message.size() < text.size()) {
        message += TruncationSuffix;
    }
    return message;
}

MessageTextLayout::Preview MessageTextLayout::GetPreview(const std::string_view text)
{
    const auto limited = FormatMenuMessage(text);
    auto lines = Wrap(limited);
    if (lines.size() > PreviewLines) {
        // Many short lines or poorly packing words can exceed four rows even
        // within the menu budget. Pack rows so no message or suffix text is lost.
        std::string packed(limited);
        for (auto& character : packed) {
            if (character == '\r' || character == '\n' || IsSpace(character)) {
                character = ' ';
            }
        }
        lines.clear();
        std::string_view remaining(packed);
        while (!remaining.empty()) {
            const auto bytes = PrefixBytes(remaining, RowWidth);
            lines.emplace_back(remaining.substr(0, bytes));
            remaining.remove_prefix(bytes);
        }
    }
    return {std::move(lines)};
}
