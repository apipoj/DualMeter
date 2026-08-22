# DualMeter

Desk display for **Claude Code** and **Codex** usage on a Waveshare **ESP32-C6-Touch-AMOLED-2.16**.

Firmware is based on [Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) (C6 HAL + BLE + LVGL). The macOS daemon also polls Codex through the local `codex app-server`, using the same account-limit flow as [CodexMeter](https://github.com/tomatoeggs/CodexMeter).

## What you see

- One **Usage** dashboard with Claude and Codex visible together
- 5-hour and 7-day utilization, reset time, and progress bar for each provider
- Tap the screen: usage dashboard ↔ splash
- Short press **PWR** on the usage dashboard: cycle brightness
- Hold **PWR** ~3s then release: Bluetooth pairing mode

## Hardware

Waveshare ESP32-C6-Touch-AMOLED-2.16 only (no PSRAM). Mount it with the three physical buttons along the top edge; firmware rotates the display and touch mapping for that orientation. Other boards are still in `firmware/platformio.ini` from upstream but are untested in this fork.

## macOS setup

Needs Claude Code signed in, Codex CLI signed in, Python 3.11+, and Bluetooth on.

```bash
cd DualMeter
./install-mac.sh
```

Flash the board (first time, factory firmware has no DualMeter identity):

```bash
python3 -m venv .venv
.venv/bin/pip install platformio
export PATH="$PWD/.venv/bin:$PATH"
./flash-mac.sh waveshare_amoled_216_c6 /dev/cu.usbmodem101
```

Then pair in **System Settings → Bluetooth → DualMeter**. The daemon looks for that name.

Serial commands without BLE (enter them in a 115200-baud monitor):

```text
demo  # show sample Claude + Codex usage
pair  # clear Bluetooth bonds and advertise for a new host
```

Use `pair` over USB when the PWR hold gesture is difficult to time; it clears
only the saved Bluetooth owner/bonds and leaves other settings intact.

## Payload

The host writes one JSON object over BLE:

```json
{
  "s": 42, "sr": 73, "w": 18, "wr": 3200, "st": "allowed", "ok": true,
  "xs": 27, "xsr": 110, "xw": 41, "xwr": 5400, "xok": true
}
```

`s/sr/w/wr` are Claude used-percent and reset minutes. `xs/xsr/xw/xwr` are Codex.

## License and credit

Clawdmeter includes Anthropic brand fonts and Clawd art. See the upstream README licensing note. DualMeter adds Codex polling under MIT-style original code; keep upstream attribution if you redistribute.
