#include "press.h"

namespace press {

void Tracker::begin(uint32_t now, bool down) {
  raw_ = down;
  stable_ = down;
  stage_ = down ? Stage::Ignored : Stage::Up;
  changedAt_ = now;
  pressedAt_ = now;
}

Event Tracker::update(uint32_t now, bool down) {
  if (down != raw_) {
    raw_ = down;
    changedAt_ = now;
  }

  // The reading has held steady long enough: the button really moved.
  if (raw_ != stable_ && now - changedAt_ >= kDebounceMs) {
    stable_ = raw_;
    if (stable_) {
      pressedAt_ = changedAt_;
      stage_ = Stage::Pressed;
      return Event::None;
    }
    const Stage stage = stage_;
    stage_ = Stage::Up;
    switch (stage) {
      case Stage::Pressed: return Event::Short;
      case Stage::Off: return Event::Off;
      case Stage::Flip: return Event::Flip;
      default: return Event::None;  // after Forget, or the press from startup
    }
  }

  if (!stable_) return Event::None;
  const uint32_t held = now - pressedAt_;
  switch (stage_) {
    case Stage::Pressed:
      if (held >= kOffMs) {
        stage_ = Stage::Off;
        return Event::ShowOff;
      }
      break;
    case Stage::Off:
      if (held >= kFlipMs) {
        stage_ = Stage::Flip;
        return Event::ShowFlip;
      }
      break;
    case Stage::Flip:
      if (held >= kForgetMs) {
        stage_ = Stage::Forgot;
        return Event::Forget;
      }
      break;
    default: break;
  }
  return Event::None;
}

}  // namespace press
