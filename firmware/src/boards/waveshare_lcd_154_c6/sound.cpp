#include "../../hal/sound_hal.h"

// ES8311 + NS4150B amp are populated (I2S MCLK 19 / BCLK 20 / LRCK 22 /
// DOUT 23, PA 15) but the chime path is unverified on C6, so this no-ops
// like the other C6 ports.

void sound_hal_init(void) {}
void sound_hal_tick(void) {}
void sound_hal_play_reset(void) {}
