#pragma once
// Turns battery readings into the percentage on the home screen, a
// low-battery warning, and a signal to turn off when the battery is empty.
// Pure logic with no Arduino calls, so it's unit tested; src/battery.cpp
// takes the readings.

#include <cstdint>

namespace charge {

// Rough charge left in a LiPo cell at this voltage, 0-100, with the glasses
// running. Treats kEmptyMillivolts as 0%.
int percentFromMillivolts(int millivolts);

constexpr int kLowPercent = 10;           // warn once at this level
constexpr int kEmptyMillivolts = 3400;    // turn off once the battery stays below this...
constexpr uint32_t kEmptyForMs = 60000;   // ...for this long, so a brief dip doesn't count
constexpr int kNoBatteryMillivolts = 2500;  // readings below this aren't a battery at all

enum class Event : uint8_t { None, Low, Empty };

class Gauge {
 public:
  // Adds a reading taken at `now` (every few seconds): the voltage, and the
  // percentage from the board's fuel gauge chip if it has one (-1 if not).
  Event add(uint32_t now, int millivolts, int chipPercent = -1);

  // What to show, in steps of 5, or -1 before the first reading.
  int percent() const { return shown_; }

 private:
  bool started_ = false;
  float millivolts_ = 0;  // smoothed
  float percent_ = 0;     // smoothed
  int shown_ = -1;
  bool warned_ = false;
  bool lowSinceSet_ = false;
  uint32_t lowSince_ = 0;
  bool emptySent_ = false;
};

}  // namespace charge
