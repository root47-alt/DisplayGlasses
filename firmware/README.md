# Firmware

C++ for the ESP32-S3, built with [PlatformIO](https://platformio.org).

## Setup

1. Install [VS Code](https://code.visualstudio.com) and the **PlatformIO IDE** extension.
2. In VS Code, **File → Open Folder** and pick this `firmware` folder (the one with `platformio.ini`). PlatformIO downloads the ESP32 tools the first time, which takes a few minutes.

## Builds

| Build | What it's for |
|---|---|
| `feather_demo` | **Start here.** Fake notifications on a timer, no Bluetooth. Tests the screen and the optics. |
| `feather` | The real thing: pairs with your iPhone ([docs/iphone-setup.md](../docs/iphone-setup.md)). |
| `feather_v2`, `feather_v2_demo` | The same two for the ESP32 Feather V2 (the in-stock alternative board). |
| `wokwi` | Demo mode for the Wokwi simulator. |
| `native` | Unit tests that run on your computer. |

## Flash the board

**Easiest:** open the [browser flasher](https://root47-alt.github.io/DisplayGlasses/flash/) in Chrome or Edge on a computer, plug in the board, and click Install. Nothing to set up.

**With PlatformIO** (needed once you change the code):

1. Plug the screen into the Feather's STEMMA QT port, then plug the Feather into your computer with a USB-C data cable.
2. In the blue PlatformIO bar at the bottom of VS Code, click the environment selector and pick `env:feather_demo` (later `env:feather`).
3. Click **→ Upload**.

From a terminal, the same thing is:

```bash
pio run -e feather_demo -t upload
```

To watch the log messages (pairing progress, incoming notifications):

```bash
pio device monitor
```

**If the upload fails** with "no serial port" or "failed to connect": hold **BOOT**, tap **RESET**, release **BOOT**, and upload again. Press **RESET** once it's done.

## Button

The glasses have one button: **SW38** on the Feather V2, **BOOT** on the ESP32-S3 Feather. Any button wired from a pin to GND also works; set `BUTTON_PIN` in `src/config.h`.

| Press | Does |
|---|---|
| Short | Dismiss the notification on screen, or bring back the last one |
| Hold until the screen says **Turn off** (2 s), then let go | Turn the glasses off |
| Keep holding until it says **Flip image** (5 s), then let go | Cycle the mirror mode (saved) |
| Hold 8 s | Forget the paired iPhone and restart |
| Press while off | Turn the glasses back on |

Off is the ESP32's deep sleep: the screen and Bluetooth are off and the board draws so little that a charged battery lasts weeks. The battery still charges over USB while the glasses are off. Turning them back on starts them up as if you'd just plugged in the battery, and they reconnect to the iPhone.

## Try it in the simulator

[Wokwi](https://wokwi.com) simulates the ESP32 and the OLED, so you can watch the HUD before the parts arrive. Bluetooth isn't simulated, so the simulator always runs demo mode.

1. Install the **Wokwi Simulator** extension in VS Code and activate its license when it asks.
2. Build the simulator firmware:

   ```bash
   pio run -e wokwi
   ```

3. Press F1 → **Wokwi: Start Simulator**. The blue button is the glasses button.

The wiring is in `diagram.json`, and `wokwi.toml` points the simulator at the build.

## Run the tests

Unit tests for the logic in `lib/core` (text, clock, the iPhone notification protocol):

```bash
pio test -e native
```

Screen tests, which run `src/hud.cpp` on your computer and check the pixels it draws (see [tools/hudsim](../tools/hudsim/README.md)):

```bash
../tools/hudsim/build.sh --test
```

Both need a C++ compiler on your computer: Xcode command line tools on a Mac, `build-essential` on Linux, or WSL on Windows. GitHub runs both, and builds every firmware version, on each push.

## Code map

| File | What it does |
|---|---|
| `src/main.cpp` | Wires everything together |
| `src/config.h` | **Settings:** pins, 12/24-hour clock, brightness, Bluetooth name |
| `src/hud.*` | Draws the screens: clock, notification pop-ups, toasts |
| `src/iphone_link.*` | Bluetooth pairing and the iPhone notification and time services, on their own task so the screen never waits |
| `src/demo.*` | Fake notifications for demo mode |
| `src/button.h` | Reads the button |
| `src/power.*` | Turning off (deep sleep) and waking up on a button press |
| `lib/core/text.*` | Cleans up and word-wraps text for the font |
| `lib/core/clock.*` | Date and time math |
| `lib/core/ancs.*` | Apple's notification protocol, byte by byte |
| `lib/core/notice.*` | Decides what each notification becomes (pop-up, weather line, Claude answer) |
| `lib/core/press.*` | What each press and hold of the button means |
| `test/test_core/` | Unit tests for `lib/core` |
| `scripts/merge_bin.py` | Makes `merged.bin` after each build, for the browser flasher |
