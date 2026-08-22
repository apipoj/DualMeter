#pragma once

#include <stdint.h>

// The C6 2.16 is mounted 90 degrees clockwise so its original left-side
// buttons become the top edge. The CO5300 cannot transpose pixels in hardware,
// so firmware writes every LVGL flush 90 degrees counter-clockwise on-panel.
namespace c6_rotation {

struct Point {
    int32_t x;
    int32_t y;
};

struct Rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

// Logical LVGL coordinate -> physical panel coordinate (90 degrees CCW).
constexpr Point logical_to_panel(int32_t x, int32_t y, int32_t side) {
    return {y, side - 1 - x};
}

// Physical touch coordinate -> logical LVGL coordinate (inverse: 90 degrees CW).
constexpr Point panel_to_logical(int32_t x, int32_t y, int32_t side) {
    return {side - 1 - y, x};
}

constexpr Rect logical_rect_to_panel(
    int32_t x, int32_t y, int32_t w, int32_t h, int32_t side) {
    return {y, side - x - w, h, w};
}

// Rotate a row-major w-by-h RGB565 rectangle 90 degrees CCW into dst.
// dst has dimensions h-by-w and must hold w*h pixels.
inline void rotate_pixels_ccw(
    const uint16_t* src, uint16_t* dst, int32_t w, int32_t h) {
    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            dst[(w - 1 - x) * h + y] = src[y * w + x];
        }
    }
}

}  // namespace c6_rotation
