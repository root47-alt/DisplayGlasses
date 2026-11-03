#pragma once
// Reads the battery. The Feather V2 measures the battery's voltage through a
// divider on an analog pin (BATTERY_PIN in config.h). The ESP32-S3 Feather
// has a MAX17048 fuel gauge chip on the I2C bus instead. Boards with neither
// show no battery level.

namespace battery {

// Call after the screen has started the I2C bus. Returns false if this board
// has no way to read its battery.
bool begin();

struct Reading {
  int millivolts = 0;  // battery voltage
  int percent = -1;    // from the fuel gauge chip, or -1 if there isn't one
};

// Returns false if the reading failed.
bool read(Reading& out);

}  // namespace battery
