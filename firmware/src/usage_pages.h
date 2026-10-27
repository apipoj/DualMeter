#pragma once

#include <stdint.h>

namespace usage_pages {

enum class Page : uint8_t {
    Splash,
    All,
    Claude,
    Codex,
    Kimi,
};

struct Availability {
    bool claude;
    bool codex;
    bool kimi = false;  // opt-in third provider; absent keeps the two-card layout
};

struct CardLayout {
    bool claude_visible;
    bool codex_visible;
    bool kimi_visible;
    int16_t card_height;
    int16_t card_y_offset;
    bool focused;
    bool compact;  // three cards share the two-card envelope
};

constexpr Page normalize(Page page, Availability available) {
    if (page == Page::Claude && !available.claude) return Page::All;
    if (page == Page::Codex && !available.codex) return Page::All;
    if (page == Page::Kimi && !available.kimi) return Page::All;
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
        if (available.kimi) return Page::Kimi;
        return Page::Splash;
    case Page::Claude:
        if (available.codex) return Page::Codex;
        if (available.kimi) return Page::Kimi;
        return Page::Splash;
    case Page::Codex:
        return available.kimi ? Page::Kimi : Page::Splash;
    case Page::Kimi:
        return Page::Splash;
    }
    return Page::All;
}

constexpr CardLayout layout(Page page, int16_t card_height, int16_t card_gap,
                            Availability available = {true, true, false}) {
    const int16_t envelope_height = (int16_t)(2 * card_height + card_gap);
    const int16_t focused_height = (int16_t)(card_height + 4 * card_gap);
    const int16_t centered_offset = (int16_t)((envelope_height - focused_height) / 2);
    const int16_t compact_height = (int16_t)((envelope_height - 2 * card_gap) / 3);
    switch (page) {
    case Page::Claude:
        return {true, false, false, focused_height, centered_offset, true, false};
    case Page::Codex:
        return {false, true, false, focused_height, centered_offset, true, false};
    case Page::Kimi:
        return {false, false, true, focused_height, centered_offset, true, false};
    default:
        if (available.kimi)
            return {true, true, true, compact_height, 0, false, true};
        return {true, true, false, card_height, 0, false, false};
    }
}

}  // namespace usage_pages
