#pragma once

// Waveshare ESP32-C6-Touch-LCD-1.54 — C6 sibling of the S3 LCD-1.54 kit.
// 240x240 ST7789 (4-wire SPI) + CST816 touch + battery ADC (no PMU — the
// ETA6098 charger is standalone) + ES8311 codec + QMI8658 IMU.
//
// Pin map from Waveshare's docs (docs.waveshare.com/ESP32-C6-Touch-LCD-1.54)
// cross-checked against their ESP-IDF BSP (esp32_c6_touch_lcd_1_54.h). An I2C
// scan on hardware found 0x18 (ES8311), 0x40 (ES7210), 0x6B (QMI8658).

#define BOARD_NAME           "Waveshare LCD 1.54 (C6)"

// ---- Display geometry ----
#define LCD_WIDTH            240
#define LCD_HEIGHT           240

// ---- SPI display pins (ST7789, 4-wire SPI) ----
#define LCD_CS               11
#define LCD_SCLK             1
#define LCD_MOSI             2
#define LCD_DC               3
#define LCD_RST              4     // also resets the touch controller
#define LCD_BL               6     // backlight, LEDC PWM (TFT has no brightness cmd)

// ---- I2C bus (touch + codec + IMU share one bus) ----
#define IIC_SDA              8
#define IIC_SCL              7

// ---- Touch (CST816, minimal inline I2C reader) ----
#define TP_INT               5
#define TP_RST               -1    // no dedicated line; pulsed via LCD_RST
#define CST816_ADDR          0x15

// ---- Battery (ADC divider, no I2C-readable PMU, no power-hold latch) ----
#define BAT_ADC_PIN          0
#define BAT_VOLT_DIVIDER     3.0f

// ---- Buttons (active-LOW GPIOs) ----
// The board's PWR key drives a power-switch IC, not a GPIO, so the PLUS key
// takes the PWR role (screens / brightness / hold-to-pair).
#define BTN_BACK_GPIO        9     // BOOT — primary, Space (PTT)
#define BTN_PWR_GPIO         18    // PLUS (KEY_PLUS) — PWR role

// ---- Capability flags ----
#define BOARD_HAS_SECONDARY_BUTTON 0
#define BOARD_HAS_ROTATION         0
#define BOARD_HAS_IMU              0    // QMI8658 present but unused
#define BOARD_HAS_BATTERY          1
#define BOARD_HAS_IO_EXPANDER      0
#define BOARD_HAS_SOUND            0    // ES8311 present; chime not wired up yet
