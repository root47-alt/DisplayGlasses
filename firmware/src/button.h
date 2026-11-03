#pragma once
// The glasses' one button, wired between a pin and GND. What each press and
// hold means is decided in lib/core/press.h.

#include <Arduino.h>

#include "press.h"

class Button {
 public:
  using Event = press::Event;

  explicit Button(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    tracker_.begin(millis(), isDown());
  }
  bool isDown() const { return digitalRead(pin_) == LOW; }

  // Call every loop(). Returns at most one event per call.
  Event update() { return tracker_.update(millis(), isDown()); }

 private:
  int pin_;
  press::Tracker tracker_;
};
