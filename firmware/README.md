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
| `wokwi` | Demo mode for the Wokwi simulator. |
| `native` | Unit tests that run on your computer. |

## Flash the board

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

The Feather's **BOOT** button is the only button (any button wired from a pin to GND works; set `BUTTON_PIN` in `src/config.h`).

| Press | Does |
|---|---|
| Short | Dismiss the notification on screen, or bring back the last one |
| Hold 1–2 s, then release | Cycle the mirror mode (saved) |
| Hold 8 s | Forget the paired iPhone and restart |

## Try it in the simulator

[Wokwi](https://wokwi.com) simulates the ESP32 and the OLED, so you can watch the HUD before the parts arrive. Bluetooth isn't simulated, so the simulator always runs demo mode.

1. Install the **Wokwi Simulator** extension in VS Code and activate its license when it asks.
2. Build the simulator firmware:

   ```bash
   pio run -e wokwi
   ```

3. Press F1 → **Wokwi: Start Simulator**. The blue button is the glasses button.

The wiring is in `diagram.json`, and `wokwi.toml` points the simulator at the build.

## Run the unit tests

```bash
pio test -e native
```

This needs a C++ compiler on your computer: Xcode command line tools on a Mac, `build-essential` on Linux, or WSL on Windows.

## Code map

| File | What it does |
|---|---|
| `src/main.cpp` | Wires everything together |
| `src/config.h` | **Settings:** pins, 12/24-hour clock, brightness, Bluetooth name |
| `src/hud.*` | Draws the screens: clock, notification pop-ups, toasts |
| `src/iphone_link.*` | Bluetooth pairing and the iPhone notification and time services |
| `src/demo.*` | Fake notifications for demo mode |
| `src/button.h` | Short, long and very long presses |
| `lib/core/text.*` | Cleans up and word-wraps text for the font |
| `lib/core/clock.*` | Date and time math |
| `lib/core/ancs.*` | Apple's notification protocol, byte by byte |
| `lib/core/notice.*` | Decides what each notification becomes (pop-up, weather line, Claude answer) |
| `test/test_core/` | Unit tests for `lib/core` |
