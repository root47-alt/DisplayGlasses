#include "clock.h"

#include <cstdio>

namespace clockutil {
namespace {

// Howard Hinnant's days_from_civil / civil_from_days algorithms.
int64_t daysFromCivil(int64_t y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

void civilFromDays(int64_t z, int& y, int& m, int& d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  y = static_cast<int>(static_cast<int64_t>(yoe) + era * 400 + (m <= 2));
}

int64_t floorDiv(int64_t a, int64_t b) {
  int64_t q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
  return q;
}

const char* const kWeekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

}  // namespace

int64_t toEpoch(int year, int month, int day, int hour, int minute, int second) {
  return daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) * 86400 +
         hour * 3600 + minute * 60 + second;
}

DateTime fromEpoch(int64_t epoch) {
  const int64_t days = floorDiv(epoch, 86400);
  const int64_t secs = epoch - days * 86400;
  DateTime t;
  civilFromDays(days, t.year, t.month, t.day);
  t.hour = static_cast<int>(secs / 3600);
  t.minute = static_cast<int>((secs % 3600) / 60);
  t.second = static_cast<int>(secs % 60);
  int64_t w = (days + 4) % 7;  // 1970-01-01 was a Thursday
  if (w < 0) w += 7;
  t.weekday = static_cast<int>(w);
  return t;
}

std::string formatTime(const DateTime& t, bool use24h) {
  char buf[8];
  if (use24h) {
    std::snprintf(buf, sizeof buf, "%02d:%02d", t.hour, t.minute);
  } else {
    const int h = t.hour % 12 == 0 ? 12 : t.hour % 12;
    std::snprintf(buf, sizeof buf, "%d:%02d", h, t.minute);
  }
  return buf;
}

const char* amPm(const DateTime& t, bool use24h) {
  if (use24h) return "";
  return t.hour < 12 ? "AM" : "PM";
}

std::string formatDate(const DateTime& t) {
  const int w = (t.weekday >= 0 && t.weekday < 7) ? t.weekday : 0;
  const int m = (t.month >= 1 && t.month <= 12) ? t.month - 1 : 0;
  char buf[16];
  std::snprintf(buf, sizeof buf, "%s %d %s", kWeekdays[w], t.day, kMonths[m]);
  return buf;
}

void Clock::set(const DateTime& t, uint32_t nowMs) {
  baseEpoch_ = toEpoch(t.year, t.month, t.day, t.hour, t.minute, t.second);
  baseMs_ = nowMs;
  valid_ = true;
}

DateTime Clock::now(uint32_t nowMs) const {
  // Unsigned subtraction stays correct when millis() wraps after ~49 days.
  const uint32_t elapsedMs = nowMs - baseMs_;
  return fromEpoch(baseEpoch_ + elapsedMs / 1000);
}

}  // namespace clockutil
