#include <array>
#include <cassert>
#include <cstdint>

#include "../src/boards/waveshare_amoled_216_c6/fixed_rotation.h"

int main() {
    constexpr int32_t side = 480;

    // The device is mounted 90 degrees clockwise so its left-side buttons
    // become the top edge. Content is compensated 90 degrees CCW on-panel.
    constexpr auto top_left = c6_rotation::logical_to_panel(0, 0, side);
    constexpr auto top_right = c6_rotation::logical_to_panel(side - 1, 0, side);
    constexpr auto bottom_left = c6_rotation::logical_to_panel(0, side - 1, side);
    static_assert(top_left.x == 0 && top_left.y == side - 1);
    static_assert(top_right.x == 0 && top_right.y == 0);
    static_assert(bottom_left.x == side - 1 && bottom_left.y == side - 1);

    // Touch is the inverse mapping, returning logical LVGL coordinates.
    constexpr auto logical = c6_rotation::panel_to_logical(top_right.x, top_right.y, side);
    static_assert(logical.x == side - 1 && logical.y == 0);

    constexpr auto rect = c6_rotation::logical_rect_to_panel(10, 20, 30, 40, side);
    static_assert(rect.x == 20 && rect.y == 440 && rect.w == 40 && rect.h == 30);

    for (int32_t y = 0; y < side; ++y) {
        for (int32_t x = 0; x < side; ++x) {
            const auto panel = c6_rotation::logical_to_panel(x, y, side);
            const auto round_trip = c6_rotation::panel_to_logical(panel.x, panel.y, side);
            assert(round_trip.x == x);
            assert(round_trip.y == y);
        }
    }

    // 2x3 source rotated CCW becomes a 3x2 destination:
    // [1 2]       [2 4 6]
    // [3 4]  ->   [1 3 5]
    // [5 6]
    const std::array<uint16_t, 6> src = {1, 2, 3, 4, 5, 6};
    std::array<uint16_t, 6> dst = {};
    c6_rotation::rotate_pixels_ccw(src.data(), dst.data(), 2, 3);
    const std::array<uint16_t, 6> expected = {2, 4, 6, 1, 3, 5};
    assert(dst == expected);

    return 0;
}
