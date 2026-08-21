#!/bin/bash
# Build and flash Clawdmeter firmware on macOS.
# Usage:
#   ./flash-mac.sh <board>                       # auto-detect /dev/cu.usbmodem*
#   ./flash-mac.sh <board> /dev/cu.usbmodem1101  # explicit USB serial port
#
# <board> is the PlatformIO env name, e.g. waveshare_amoled_216 or waveshare_amoled_18.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BOARD="$1"
PORT="$2"

if [ -z "$BOARD" ]; then
    BOARD="waveshare_amoled_216_c6"
    echo "No board given — defaulting to $BOARD (ESP32-C6-Touch-AMOLED-2.16)"
fi

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "Error: no /dev/cu.usbmodem* device found. Plug in via USB-C."
        exit 1
    fi
fi

PIO=""
if [ -x "$SCRIPT_DIR/.venv/bin/pio" ]; then
    PIO="$SCRIPT_DIR/.venv/bin/pio"
elif command -v pio >/dev/null; then
    PIO="$(command -v pio)"
else
    echo "Error: 'pio' not found. Install with:"
    echo "  python3 -m venv .venv && .venv/bin/pip install platformio"
    exit 1
fi

echo "=== Flashing DualMeter ==="
echo "Board: $BOARD"
echo "Port:  $PORT"
echo ""

cd "$SCRIPT_DIR/firmware"
"$PIO" run -e "$BOARD" -t upload --upload-port "$PORT"

echo ""
echo "=== Done ==="
echo "Monitor with: pio device monitor -p $PORT -b 115200"
