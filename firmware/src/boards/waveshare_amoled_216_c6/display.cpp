#include "../../hal/display_hal.h"
#include "board.h"
#include "fixed_rotation.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// C6 AMOLED-2.16 uses a CO5300 AMOLED panel (per the Waveshare
// ESP32-C6-Touch-AMOLED-2.16 spec) — the same controller as the S3
// AMOLED-2.16 sibling, so we drive it with Arduino_CO5300 and reuse that
// class's vendor-correct init rather than the SH8601 class + a hand-patched
// sequence. LCD reset is not wired to any MCU GPIO; the panel boots from its
// internal power-on reset (rst = GFX_NOT_DEFINED).
//
// The device is desk-mounted 90 degrees clockwise so its original left-edge
// buttons sit along the top. CO5300 cannot transpose pixels in hardware, so
// every LVGL partial flush is rotated 90 degrees CCW into one internal-SRAM
// strip. This is fixed mounting compensation, not IMU-driven rotation.

#define ROT_BUF_LINES 20
static uint16_t rot_buf[LCD_WIDTH * ROT_BUF_LINES];
static_assert(LCD_WIDTH == LCD_HEIGHT,
              "Fixed C6 rotation currently requires a square panel");
static_assert(sizeof(rot_buf) == LCD_WIDTH * ROT_BUF_LINES * sizeof(uint16_t),
              "Rotation strip must match the C6 LVGL partial buffer capacity");

static Arduino_DataBus* bus = nullptr;
static Arduino_CO5300*  gfx = nullptr;

void display_hal_init(void) {
    bus = new Arduino_ESP32QSPI(
        LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
    // CO5300 constructor: (bus, rst, rotation, w, h, col_off1..2, row_off1..2).
    // No reset GPIO on this board; the 480-wide panel is full-width so all
    // offsets are 0 — matches the S3 AMOLED-2.16 instantiation.
    gfx = new Arduino_CO5300(
        bus, GFX_NOT_DEFINED, 0 /* rotation disabled */,
        LCD_WIDTH, LCD_HEIGHT, 0, 0, 0, 0);
}

// Arduino_CO5300::begin() already issues SLPOUT, SPI-mode control, pixel
// format, brightness-control, DISPON and a default MADCTL. The ONLY thing it
// does not set is this panel's manufacturer page-0x20 driving-voltage
// registers (0x19/0x1C) — without them the panel stays black even with the
// rails up. Set just those; everything else the SH8601-era hack also wrote
// (0xC4/0x36/0x53/0x51/0x63/0x29) is now covered by the class init.
//
// Keep the CO5300 at its rotation-0 MADCTL. Its hardware supports X/Y flips
// but not the transpose needed for a quarter turn; display_hal_draw_bitmap()
// performs the fixed 90-degree compensation instead.
static void send_panel_driving_init(Arduino_DataBus* b) {
    b->beginWrite();
    b->writeC8D8(0xFE, 0x20);    // enter manufacturer command page 0x20
    b->writeC8D8(0x19, 0x10);    // panel driving voltage
    b->writeC8D8(0x1C, 0xA0);    // panel driving voltage
    b->writeC8D8(0xFE, 0x00);    // back to user command page
    b->endWrite();
    delay(20);
}

void display_hal_begin(void) {
    gfx->begin();
    send_panel_driving_init(bus);   // panel-specific regs the class init omits
    gfx->fillScreen(0x0000);
    gfx->setBrightness(200);
}

void display_hal_set_brightness(uint8_t level) {
    if (gfx) gfx->setBrightness(level);
}

void display_hal_fill_screen(uint16_t color) {
    if (gfx) gfx->fillScreen(color);
}

void display_hal_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h,
                             const uint16_t* pixels) {
    if (!gfx) return;

    const int32_t pixel_count = w * h;
    if (pixel_count > (int32_t)(sizeof(rot_buf) / sizeof(rot_buf[0]))) {
        Serial.printf("Rotation buffer overflow: %ld pixels\n", (long)pixel_count);
        return;
    }

    c6_rotation::rotate_pixels_ccw(pixels, rot_buf, w, h);
    const c6_rotation::Rect dst =
        c6_rotation::logical_rect_to_panel(x, y, w, h, LCD_WIDTH);
    gfx->draw16bitRGBBitmap(dst.x, dst.y, rot_buf, dst.w, dst.h);
}

void display_hal_tick(void) {
    // No rotation cycle on this board.
}

// CO5300 requires even-aligned flush regions.
void display_hal_round_area(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2) {
    *x1 = *x1 & ~1;
    *y1 = *y1 & ~1;
    *x2 = *x2 | 1;
    *y2 = *y2 | 1;
}
