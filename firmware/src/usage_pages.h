#pragma once

#include <stdint.h>

namespace usage_pages {

enum class Page : uint8_t {
    Splash,
    All,
    Claude,
    Codex,
};

struct Availability {
    bool claude;
    bool codex;
};

struct CardLayout {
    bool claude_visible;
    bool codex_visible;
    int16_t card_height;
    int16_t card_y_offset;
    bool focused;
};

constexpr Page normalize(Page page, Availability available) {
    if (page == Page::Claude && !available.claude) return Page::All;
    if (page == Page::Codex && !available.codex) return Page::All;
    return page;
}

constexpr Page next(Page page, Availability available) {
    page = normalize(page, available);
    switch (page) {
    case Page::Splash:
        return Page::All;
    case Page::All:
        if (available.claude) return Page::Claude;
        if (available.codex) return Page::Codex;
        return Page::Splash;
    case Page::Claude:
        return available.codex ? Page::Codex : Page::Splash;
    case Page::Codex:
        return Page::Splash;
    }
    return Page::All;
}

constexpr CardLayout layout(Page page, int16_t card_height, int16_t card_gap) {
    const int16_t envelope_height = (int16_t)(2 * card_height + card_gap);
    const int16_t focused_height = (int16_t)(card_height + 4 * card_gap);
    const int16_t centered_offset = (int16_t)((envelope_height - focused_height) / 2);
    switch (page) {
    case Page::Claude:
        return {true, false, focused_height, centered_offset, true};
    case Page::Codex:
        return {false, true, focused_height, centered_offset, true};
    default:
        return {true, true, card_height, 0, false};
    }
}

}  // namespace usage_pages
