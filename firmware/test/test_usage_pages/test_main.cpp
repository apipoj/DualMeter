#include <cstdio>

#include "../../src/usage_pages.h"

static int failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
            ++failures;                                                        \
        }                                                                      \
    } while (0)

using usage_pages::Availability;
using usage_pages::Page;

static void should_cycle_all_providers_then_splash() {
    constexpr Availability both{true, true};
    CHECK(usage_pages::next(Page::All, both) == Page::Claude);
    CHECK(usage_pages::next(Page::Claude, both) == Page::Codex);
    CHECK(usage_pages::next(Page::Codex, both) == Page::Splash);
    CHECK(usage_pages::next(Page::Splash, both) == Page::All);
}

static void should_skip_unavailable_provider_pages() {
    constexpr Availability claude_only{true, false};
    CHECK(usage_pages::next(Page::All, claude_only) == Page::Claude);
    CHECK(usage_pages::next(Page::Claude, claude_only) == Page::Splash);

    constexpr Availability codex_only{false, true};
    CHECK(usage_pages::next(Page::All, codex_only) == Page::Codex);
    CHECK(usage_pages::next(Page::Codex, codex_only) == Page::Splash);

    constexpr Availability neither{false, false};
    CHECK(usage_pages::next(Page::All, neither) == Page::Splash);
}

static void should_normalize_a_page_when_its_provider_disappears() {
    CHECK(usage_pages::normalize(Page::Claude, {false, true}) == Page::All);
    CHECK(usage_pages::normalize(Page::Codex, {true, false}) == Page::All);
    CHECK(usage_pages::normalize(Page::Claude, {true, false}) == Page::Claude);
    CHECK(usage_pages::normalize(Page::Splash, {false, false}) == Page::Splash);
}

static void should_compose_a_focused_card_inside_the_two_card_envelope() {
    constexpr auto all = usage_pages::layout(Page::All, 144, 12);
    CHECK(all.claude_visible && all.codex_visible && !all.kimi_visible);
    CHECK(all.card_height == 144);
    CHECK(all.card_y_offset == 0);
    CHECK(!all.focused);

    constexpr auto claude = usage_pages::layout(Page::Claude, 144, 12);
    CHECK(claude.claude_visible && !claude.codex_visible);
    CHECK(claude.card_height == 192);
    CHECK(claude.card_y_offset == 54);
    CHECK(claude.focused);

    constexpr auto codex_small = usage_pages::layout(Page::Codex, 74, 6);
    CHECK(!codex_small.claude_visible && codex_small.codex_visible);
    CHECK(codex_small.card_height == 98);
    CHECK(codex_small.card_y_offset == 28);
    CHECK(codex_small.focused);
}

static void should_cycle_through_kimi_when_it_is_available() {
    constexpr Availability all3{true, true, true};
    CHECK(usage_pages::next(Page::All, all3) == Page::Claude);
    CHECK(usage_pages::next(Page::Claude, all3) == Page::Codex);
    CHECK(usage_pages::next(Page::Codex, all3) == Page::Kimi);
    CHECK(usage_pages::next(Page::Kimi, all3) == Page::Splash);

    constexpr Availability kimi_only{false, false, true};
    CHECK(usage_pages::next(Page::All, kimi_only) == Page::Kimi);
    CHECK(usage_pages::next(Page::Kimi, kimi_only) == Page::Splash);

    constexpr Availability claude_kimi{true, false, true};
    CHECK(usage_pages::next(Page::Claude, claude_kimi) == Page::Kimi);

    CHECK(usage_pages::normalize(Page::Kimi, {true, true}) == Page::All);
    CHECK(usage_pages::normalize(Page::Kimi, {true, true, true}) == Page::Kimi);
}

static void should_fit_three_compact_cards_in_the_two_card_envelope() {
    constexpr auto two = usage_pages::layout(Page::All, 144, 12, {true, true, false});
    CHECK(!two.kimi_visible && !two.compact);
    CHECK(two.card_height == 144);

    constexpr auto three = usage_pages::layout(Page::All, 144, 12, {true, true, true});
    CHECK(three.claude_visible && three.codex_visible && three.kimi_visible);
    CHECK(three.compact && !three.focused);
    CHECK(three.card_height == 92);
    // 3 cards + 2 gaps never overflow the 2-card envelope (2*144 + 12).
    CHECK(3 * three.card_height + 2 * 12 <= 2 * 144 + 12);

    constexpr auto kimi = usage_pages::layout(Page::Kimi, 144, 12, {true, true, true});
    CHECK(!kimi.claude_visible && !kimi.codex_visible && kimi.kimi_visible);
    CHECK(kimi.focused && !kimi.compact);
    CHECK(kimi.card_height == 192);
}

int main() {
    should_cycle_through_kimi_when_it_is_available();
    should_fit_three_compact_cards_in_the_two_card_envelope();
    should_cycle_all_providers_then_splash();
    should_skip_unavailable_provider_pages();
    should_normalize_a_page_when_its_provider_disappears();
    should_compose_a_focused_card_inside_the_two_card_envelope();

    if (failures) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all usage-page checks passed\n");
    return 0;
}
