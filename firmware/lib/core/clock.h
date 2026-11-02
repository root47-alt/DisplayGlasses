#pragma once
// Calendar math for the HUD clock. Pure C++ (no Arduino).
//
// There are no time zones here on purpose: the iPhone sends its local time,
// and the glasses count forward from it.

#include <cstdint>
#include <string>

namespace clockutil {

struct DateTime {
  int year = 1970;
  int month = 1;    // 1-12
  int day = 1;      // 1-31
  int hour = 0;     // 0-23
  int minute = 0;   // 0-59
  int second = 0;   // 0-59
  int weekday = 4;  // 0 = Sunday ... 6 = Saturday
};

// Seconds since 1970-01-01 00:00:00 for a wall-clock date and time.
int64_t toEpoch(int year, int month, int day, int hour, int minute, int second);

// The inverse of toEpoch, including the day of the week.
DateTime fromEpoch(int64_t epoch);

// "9:41" in 12-hour mode, "09:41" in 24-hour mode.
std::string formatTime(const DateTime& t, bool use24h);

// "AM" or "PM", or "" in 24-hour mode.
const char* amPm(const DateTime& t, bool use24h);

// "Mon 2 Nov"
std::string formatDate(const DateTime& t);

// Keeps time between updates from the phone using a millisecond counter
// (millis() on the ESP32).
class Clock {
 public:
  void set(const DateTime& t, uint32_t nowMs);
  bool valid() const { return valid_; }
  DateTime now(uint32_t nowMs) const;

 private:
  bool valid_ = false;
  int64_t baseEpoch_ = 0;
  uint32_t baseMs_ = 0;
};

}  // namespace clockutil
