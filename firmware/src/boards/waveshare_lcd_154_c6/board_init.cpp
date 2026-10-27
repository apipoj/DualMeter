#include "board.h"
#include <Arduino.h>
#include <Wire.h>

// Bring up the shared I2C bus. No IO expander and no battery power-hold
// latch on this kit — the PWR key's switch IC owns the rail.
extern "C" void board_init(void) {
    Wire.begin(IIC_SDA, IIC_SCL);
}
