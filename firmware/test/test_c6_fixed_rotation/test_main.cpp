#include <array>
#include <cstdint>
#include <cstdio>

#include "../../src/boards/waveshare_amoled_216_c6/fixed_rotation.h"

static int failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
            ++failures;                                                        \
        }                                                                      \
    } while (0)

static constexpr int32_t SIDE = 480;

static void check_rect_pixel_composition(
    int32_t sx, int32_t sy, int32_t w, int32_t h) {
    const auto dst = c6_rotation::logical_rect_to_panel(sx, sy, w, h, SIDE);
    CHECK(dst.x >= 0);
    CHECK(dst.y >= 0);
    CHECK(dst.x + dst.w <= SIDE);
    CHECK(dst.y + dst.h <= SIDE);

    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            // rotate_pixels_ccw writes this source pixel at local destination
            // (y, w-1-x). It must agree with the absolute point transform.
            const auto panel = c6_rotation::logical_to_panel(sx + x, sy + y, SIDE);
            CHECK(panel.x == dst.x + y);
            CHECK(panel.y == dst.y + (w - 1 - x));

            const auto logical =
                c6_rotation::panel_to_logical(panel.x, panel.y, SIDE);
            CHECK(logical.x == sx + x);
            CHECK(logical.y == sy + y);
        }
    }
}

static void should_map_corners_and_touch_inverse() {
    constexpr auto top_left = c6_rotation::logical_to_panel(0, 0, SIDE);
    constexpr auto top_right = c6_rotation::logical_to_panel(SIDE - 1, 0, SIDE);
    constexpr auto bottom_left = c6_rotation::logical_to_panel(0, SIDE - 1, SIDE);
    CHECK(top_left.x == 0 && top_left.y == SIDE - 1);
    CHECK(top_right.x == 0 && top_right.y == 0);
    CHECK(bottom_left.x == SIDE - 1 && bottom_left.y == SIDE - 1);

    for (int32_t y = 0; y < SIDE; ++y) {
        for (int32_t x = 0; x < SIDE; ++x) {
            const auto panel = c6_rotation::logical_to_panel(x, y, SIDE);
            const auto logical =
                c6_rotation::panel_to_logical(panel.x, panel.y, SIDE);
            CHECK(logical.x == x);
            CHECK(logical.y == y);
        }
    }
}

static void should_rotate_rectangular_pixel_order_ccw() {
    // [1 2]       [2 4 6]
    // [3 4]  ->   [1 3 5]
    // [5 6]
    const std::array<uint16_t, 6> src = {1, 2, 3, 4, 5, 6};
    std::array<uint16_t, 6> dst = {};
    c6_rotation::rotate_pixels_ccw(src.data(), dst.data(), 2, 3);
    const std::array<uint16_t, 6> expected = {2, 4, 6, 1, 3, 5};
    CHECK(dst == expected);
}

static void should_compose_rect_and_pixel_mapping_for_real_flush_shapes() {
    check_rect_pixel_composition(0, 0, 480, 480);
    check_rect_pixel_composition(10, 20, 30, 40);
    check_rect_pixel_composition(479, 479, 1, 1);

    for (int32_t y = 0; y < SIDE; y += 20) {
        check_rect_pixel_composition(0, y, SIDE, 20);  // C6 LVGL partial flush
    }
    for (int32_t y = 0; y < SIDE; y += 8) {
        check_rect_pixel_composition(0, y, SIDE, 8);   // direct splash band
    }
}

int main() {
    should_map_corners_and_touch_inverse();
    should_rotate_rectangular_pixel_order_ccw();
    should_compose_rect_and_pixel_mapping_for_real_flush_shapes();

    if (failures) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all C6 fixed-rotation checks passed\n");
    return 0;
}
