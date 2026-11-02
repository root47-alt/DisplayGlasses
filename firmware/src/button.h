#pragma once
// One push button wired between a pin and GND.
//
//   Short press  (< 1 s)          dismiss / show the last notification again
//   Long press   (1-8 s, release) cycle the mirror mode
//   Very long    (hold 8 s)       forget the paired iPhone

#include <Arduino.h>

class Button {
 public:
  enum class Event : uint8_t { None, Short, Long, VeryLong };

  explicit Button(int pin) : pin_(pin) {}

  void begin() { pinMode(pin_, INPUT_PULLUP); }
  bool isDown() const { return digitalRead(pin_) == LOW; }

  // Call every loop(). Returns at most one event per call.
  Event update() {
    const uint32_t now = millis();
    const bool raw = isDown();
    if (raw != raw_) {
      raw_ = raw;
      changedAt_ = now;
    }
    if (now - changedAt_ < kDebounceMs || raw == stable_) {
      if (stable_ && !veryLongFired_ && now - pressedAt_ >= kVeryLongMs) {
        veryLongFired_ = true;
        return Event::VeryLong;
      }
      return Event::None;
    }
    stable_ = raw;
    if (stable_) {  // pressed
      pressedAt_ = now;
      veryLongFired_ = false;
      return Event::None;
    }
    if (veryLongFired_) return Event::None;  // released after a very long hold
    return now - pressedAt_ >= kLongMs ? Event::Long : Event::Short;
  }

 private:
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint32_t kLongMs = 1000;
  static constexpr uint32_t kVeryLongMs = 8000;

  int pin_;
  bool raw_ = false;
  bool stable_ = false;
  bool veryLongFired_ = false;
  uint32_t changedAt_ = 0;
  uint32_t pressedAt_ = 0;
};
