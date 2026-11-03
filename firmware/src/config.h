#pragma once
// Settings you might want to change. Pins default to the Adafruit ESP32-S3
// Feather with the OLED plugged into its STEMMA QT port; the wokwi build in
// platformio.ini overrides them.

// ---------- Pins ----------
#ifndef I2C_SDA
#define I2C_SDA SDA  // the board's default I2C pins (STEMMA QT on the Feather)
#endif
#ifndef I2C_SCL
#define I2C_SCL SCL
#endif
#ifndef BUTTON_PIN
#define BUTTON_PIN 0  // the Feather's BOOT button; any pin wired to GND works
#endif

// ---------- Battery ----------
// The Feather V2 reads its battery through a voltage divider on BATT_MONITOR
// (A13). The ESP32-S3 Feather has a fuel gauge chip instead, found by itself.
// For another board with a divider, set the pin and the divider's ratio.
#if !defined(BATTERY_PIN) && defined(BATT_MONITOR)
#define BATTERY_PIN BATT_MONITOR
#endif
#ifndef BATTERY_DIVIDER
#define BATTERY_DIVIDER 2
#endif

// ---------- Behaviour ----------
// Bluetooth name shown in iPhone Settings > Bluetooth.
#define DEVICE_NAME "DisplayGlasses"

// 12-hour clock with AM/PM, or 24-hour.
#define USE_24H_CLOCK false

// OLED brightness, 0-255. The beam splitter loses about half the light, so
// start at full brightness.
#define SCREEN_BRIGHTNESS 255

// Mirror mode on first boot (0 none, 1 left-right, 2 up-down, 3 rotate 180).
// Holding the button about 5 seconds cycles through them, and the choice is
// saved.
#define DEFAULT_MIRROR 0
