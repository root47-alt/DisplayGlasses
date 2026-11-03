#include "charge.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace charge {

namespace {

// A typical LiPo discharge curve at a light load: {millivolts, percent}.
struct Point {
  int mv;
  int pct;
};
constexpr Point kCurve[] = {
    {kEmptyMillivolts, 0}, {3610, 5},  {3690, 10}, {3710, 15}, {3730, 20}, {3750, 25}, {3770, 30},
    {3790, 35},            {3800, 40}, {3820, 45}, {3840, 50}, {3850, 55}, {3870, 60}, {3910, 65},
    {3950, 70},            {3980, 75}, {4020, 80}, {4080, 85}, {4110, 90}, {4150, 95}, {4200, 100},
};

// Each new reading moves the smoothed values a quarter of the way. With a
// reading every 5 seconds, that evens out the noise over about 20 seconds.
constexpr float kSmoothing = 0.25f;

// The number on screen moves in steps of 5, and only once the charge is
// clearly past the step, so it doesn't flick back and forth.
constexpr float kSlack = 4.0f;

int roundTo5(float pct) { return std::clamp(static_cast<int>(std::lround(pct / 5.0f)) * 5, 0, 100); }

}  // namespace

int percentFromMillivolts(int millivolts) {
  if (millivolts <= kCurve[0].mv) return 0;
  for (size_t i = 1; i < std::size(kCurve); ++i) {
    const Point& a = kCurve[i - 1];
    const Point& b = kCurve[i];
    if (millivolts <= b.mv) return a.pct + (millivolts - a.mv) * (b.pct - a.pct) / (b.mv - a.mv);
  }
  return 100;
}

Event Gauge::add(uint32_t now, int millivolts, int chipPercent) {
  if (millivolts < kNoBatteryMillivolts) return Event::None;
  const bool first = !started_;
  started_ = true;
  millivolts_ = first ? static_cast<float>(millivolts) : millivolts_ + (millivolts - millivolts_) * kSmoothing;

  if (chipPercent >= 0) {
    // The fuel gauge chip already knows the charge.
    const float chip = static_cast<float>(std::min(chipPercent, 100));
    percent_ = first ? chip : percent_ + (chip - percent_) * kSmoothing;
  } else {
    percent_ = static_cast<float>(percentFromMillivolts(static_cast<int>(std::lround(millivolts_))));
  }
  if (shown_ < 0 || std::fabs(percent_ - shown_) >= kSlack) shown_ = roundTo5(percent_);

  if (millivolts_ <= kEmptyMillivolts) {
    if (!lowSinceSet_) {
      lowSinceSet_ = true;
      lowSince_ = now;
    } else if (!emptySent_ && now - lowSince_ >= kEmptyForMs) {
      emptySent_ = true;
      return Event::Empty;
    }
  } else {
    lowSinceSet_ = false;
  }

  if (!warned_ && shown_ <= kLowPercent) {
    warned_ = true;
    return Event::Low;
  }
  if (warned_ && shown_ >= kLowPercent + 10) warned_ = false;  // charged again: warn next time too
  return Event::None;
}

}  // namespace charge
