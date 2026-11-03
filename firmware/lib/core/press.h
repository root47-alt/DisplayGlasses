#pragma once
// Turns the button's raw up/down readings into presses. Pure logic with no
// Arduino calls, so it's unit tested; src/button.h feeds it the pin and the
// clock.
//
//   Let go before 2 s          Short     dismiss / show the last notification again
//   Hold 2 s                   ShowOff   (the screen says "Turn off")
//   Let go between 2 and 5 s   Off       turn the glasses off
//   Hold 5 s                   ShowFlip  (the screen says "Flip image")
//   Let go between 5 and 8 s   Flip      cycle the mirror mode
//   Hold 8 s                   Forget    forget the paired iPhone, right away
//
// What happens when you let go always matches the message on the screen.

#include <cstdint>

namespace press {

enum class Event : uint8_t { None, Short, ShowOff, Off, ShowFlip, Flip, Forget };

constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kOffMs = 2000;
constexpr uint32_t kFlipMs = 5000;
constexpr uint32_t kForgetMs = 8000;

class Tracker {
 public:
  // `down` is the button's state at startup. A press that's already in
  // progress (the one that just turned the glasses on) is ignored until
  // it's let go.
  void begin(uint32_t now, bool down);

  // Call often with the raw reading. Returns at most one event per call.
  Event update(uint32_t now, bool down);

 private:
  enum class Stage : uint8_t { Up, Ignored, Pressed, Off, Flip, Forgot };

  bool raw_ = false;
  bool stable_ = false;
  Stage stage_ = Stage::Up;
  uint32_t changedAt_ = 0;
  uint32_t pressedAt_ = 0;
};

}  // namespace press
