#include "hud.h"

#include <Wire.h>

#include <algorithm>

#include "text.h"

namespace {

const uint8_t* const kFontText = u8g2_font_6x10_tf;    // 21 characters per line
const uint8_t* const kFontBold = u8g2_font_helvB08_tf;  // titles and headers
const uint8_t* const kFontClock = u8g2_font_logisoso28_tn;

constexpr int kWidth = 128;
constexpr int kTextWidth = 122;  // leaves room for the scroll bar
constexpr int kHeaderHeight = 11;
constexpr int kLineHeight = 10;
constexpr size_t kVisibleLines = 5;
constexpr size_t kMaxQueued = 8;

// Reading pace for pop-ups. Claude answers are longer, so they go slower.
constexpr uint32_t kFirstPageMs = 3000;
constexpr uint32_t kLineMs = 1500;
constexpr uint32_t kEndHoldMs = 4000;
constexpr uint32_t kCallHoldMs = 30000;  // until the phone says the call ended

float paceFor(NoticeKind kind) { return kind == NoticeKind::Claude ? 1.3f : 1.0f; }

bool reached(uint32_t now, uint32_t when) { return static_cast<int32_t>(now - when) >= 0; }

}  // namespace

const char* mirrorName(Mirror m) {
  switch (m) {
    case Mirror::None: return "none";
    case Mirror::LeftRight: return "left-right";
    case Mirror::UpDown: return "up-down";
    case Mirror::Rotate180: return "rotate 180";
    default: return "?";
  }
}

bool Hud::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  Wire.setClock(400000);
  uint8_t address = 0;
  for (uint8_t candidate : {0x3C, 0x3D}) {  // cheap modules use 0x3C, Adafruit's 0.96" uses 0x3D
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address = candidate;
      break;
    }
  }
  if (address == 0) return false;

  oled_.setI2CAddress(address * 2);  // U8g2 wants the 8-bit address
  oled_.setBusClock(400000);
  oled_.begin();
  present_ = true;
  setMirror(mirror_);
  dirty_ = true;
  return true;
}

void Hud::setMirror(Mirror m) {
  mirror_ = m;
  switch (m) {
    case Mirror::LeftRight: oled_.setDisplayRotation(U8G2_MIRROR); break;
    case Mirror::UpDown: oled_.setDisplayRotation(U8G2_MIRROR_VERTICAL); break;
    case Mirror::Rotate180: oled_.setDisplayRotation(U8G2_R2); break;
    default: oled_.setDisplayRotation(U8G2_R0); break;
  }
  dirty_ = true;
}

void Hud::setBrightness(uint8_t level) {
  if (present_) oled_.setContrast(level);
}

void Hud::setClock(const clockutil::Clock* clock, bool use24h) {
  clock_ = clock;
  use24h_ = use24h;
  dirty_ = true;
}

void Hud::setLink(Link link) {
  if (link != link_) dirty_ = true;
  link_ = link;
}

void Hud::setWeather(const std::string& weather) {
  weather_ = weather;
  dirty_ = true;
}

void Hud::push(const Notice& n) {
  const uint32_t now = millis();
  if (!showing_) {
    startNotice(n, now);
    return;
  }
  if (n.kind == NoticeKind::Call) {
    // The interrupted notification goes back to the front of the line; an
    // older call is simply replaced by the newer one.
    if (current_.kind != NoticeKind::Call) queue_.push_front(current_);
    while (queue_.size() > kMaxQueued) queue_.pop_back();
    startNotice(n, now);
    return;
  }
  if (queue_.size() >= kMaxQueued) queue_.pop_front();
  queue_.push_back(n);
}

void Hud::remove(uint32_t id) {
  if (id == 0) return;
  queue_.erase(std::remove_if(queue_.begin(), queue_.end(), [id](const Notice& n) { return n.id == id; }),
               queue_.end());
  if (hasLast_ && last_.id == id && last_.kind == NoticeKind::Call) hasLast_ = false;
  if (showing_ && current_.id == id) finishNotice(millis());
}

void Hud::toast(const std::string& message, uint32_t ms) {
  toast_ = message;
  toastUntil_ = millis() + ms;
  toastActive_ = true;
  dirty_ = true;
}

void Hud::splash(const char* title, const char* detail) {
  if (!present_) return;
  oled_.clearBuffer();
  oled_.setFont(kFontBold);
  drawCentered(fit(title, kWidth - 4), 28);
  oled_.setFont(kFontText);
  drawCentered(fit(detail, kWidth - 4), 44);
  oled_.sendBuffer();
  dirty_ = true;  // the next loop() redraws the normal screen
}

void Hud::onShortPress() {
  const uint32_t now = millis();
  if (showing_) {
    finishNotice(now);
  } else if (hasLast_) {
    startNotice(last_, now);
  }
}

void Hud::startNotice(const Notice& n, uint32_t now) {
  current_ = n;
  last_ = n;
  hasLast_ = true;
  showing_ = true;
  scroll_ = 0;

  lines_.clear();
  oled_.setFont(kFontBold);
  for (const std::string& l : text::wrap(n.title, kTextWidth, [this](const std::string& s) { return textWidth(s); })) {
    lines_.push_back({true, l});
  }
  oled_.setFont(kFontText);
  for (const std::string& l : text::wrap(n.body, kTextWidth, [this](const std::string& s) { return textWidth(s); })) {
    lines_.push_back({false, l});
  }

  const float pace = paceFor(n.kind);
  if (n.kind == NoticeKind::Call) {
    endAt_ = now + kCallHoldMs;
    endSet_ = true;
  } else if (lines_.size() <= kVisibleLines) {
    const uint32_t chars = static_cast<uint32_t>(text::length(n.title) + text::length(n.body));
    const uint32_t hold = std::min<uint32_t>(12000, std::max<uint32_t>(4000, 2500 + 60 * chars));
    endAt_ = now + static_cast<uint32_t>(hold * pace);
    endSet_ = true;
  } else {
    endSet_ = false;  // decided once the last line scrolls into view
    nextStepAt_ = now + static_cast<uint32_t>(kFirstPageMs * pace);
  }
  dirty_ = true;
}

void Hud::finishNotice(uint32_t now) {
  if (!queue_.empty()) {
    const Notice next = queue_.front();
    queue_.pop_front();
    startNotice(next, now);
    return;
  }
  showing_ = false;
  lines_.clear();
  dirty_ = true;
}

void Hud::loop() {
  const uint32_t now = millis();

  if (showing_) {
    const float pace = paceFor(current_.kind);
    if (!endSet_ && reached(now, nextStepAt_)) {
      ++scroll_;
      dirty_ = true;
      if (scroll_ + kVisibleLines >= lines_.size()) {
        endAt_ = now + static_cast<uint32_t>(kEndHoldMs * pace);
        endSet_ = true;
      } else {
        nextStepAt_ = now + static_cast<uint32_t>(kLineMs * pace);
      }
    }
    if (endSet_ && reached(now, endAt_)) finishNotice(now);
  }

  if (toastActive_ && reached(now, toastUntil_)) {
    toastActive_ = false;
    dirty_ = true;
  }

  // Redraw whenever the minute shown changes (including a corrected time).
  if (clock_ != nullptr && clock_->valid()) {
    const clockutil::DateTime t = clock_->now(now);
    const int64_t key = ((static_cast<int64_t>(t.year) * 13 + t.month) * 32 + t.day) * 1440 + t.hour * 60 + t.minute;
    if (key != lastClockKey_) {
      lastClockKey_ = key;
      dirty_ = true;
    }
  }

  if (!dirty_ || !present_) return;
  dirty_ = false;
  oled_.clearBuffer();
  if (toastActive_) {
    drawToast();
  } else if (showing_) {
    drawNotice();
  } else {
    drawHome(now);
  }
  oled_.sendBuffer();
}

void Hud::drawHome(uint32_t now) {
  const bool haveTime = clock_ != nullptr && clock_->valid();
  const clockutil::DateTime t = haveTime ? clock_->now(now) : clockutil::DateTime{};

  // Shift everything by a pixel every few minutes so the clock doesn't burn
  // into the OLED.
  const int dx = haveTime ? (t.minute % 3) - 1 : 0;
  const int dy = haveTime ? ((t.minute / 3) % 3) - 1 : 0;

  oled_.setFont(kFontText);
  if (haveTime) oled_.drawUTF8(2 + dx, 9 + dy, clockutil::formatDate(t).c_str());

  const char* status = "";
  switch (link_) {
    case Link::Demo: status = "DEMO"; break;
    case Link::Searching: status = "no phone"; break;
    case Link::Pairing: status = "pairing..."; break;
    case Link::Ready: status = ""; break;
  }
  if (*status) oled_.drawUTF8(kWidth - 2 - textWidth(status) + dx, 9 + dy, status);

  // Big clock, with AM/PM tucked to the right.
  const std::string time = haveTime ? clockutil::formatTime(t, use24h_) : "--:--";
  const char* suffix = haveTime ? clockutil::amPm(t, use24h_) : "";
  oled_.setFont(kFontClock);
  const int timeWidth = textWidth(time);
  oled_.setFont(kFontText);
  const int suffixWidth = *suffix ? textWidth(suffix) + 2 : 0;
  const int x = (kWidth - timeWidth - suffixWidth) / 2 + dx;
  oled_.setFont(kFontClock);
  oled_.drawUTF8(x, 46 + dy, time.c_str());
  if (*suffix) {
    oled_.setFont(kFontText);
    oled_.drawUTF8(x + timeWidth + 2, 25 + dy, suffix);
  }

  if (!weather_.empty()) {
    oled_.setFont(kFontText);
    drawCentered(fit(weather_, kWidth - 4), 61 + dy);
  }
}

void Hud::drawNotice() {
  // Header bar: app name on the left, clock on the right.
  oled_.drawBox(0, 0, kWidth, kHeaderHeight);
  oled_.setDrawColor(0);
  oled_.setFont(kFontText);
  std::string clockText;
  if (clock_ != nullptr && clock_->valid()) clockText = clockutil::formatTime(clock_->now(millis()), use24h_);
  const int clockWidth = clockText.empty() ? 0 : textWidth(clockText);
  if (clockWidth) oled_.drawUTF8(kWidth - 2 - clockWidth, 9, clockText.c_str());
  oled_.setFont(kFontBold);
  oled_.drawUTF8(2, 9, fit(current_.app, kWidth - 8 - clockWidth).c_str());
  oled_.setDrawColor(1);

  for (size_t i = 0; i < kVisibleLines && scroll_ + i < lines_.size(); ++i) {
    const Line& line = lines_[scroll_ + i];
    oled_.setFont(line.bold ? kFontBold : kFontText);
    oled_.drawUTF8(2, kHeaderHeight + kLineHeight * static_cast<int>(i + 1), line.text.c_str());
  }

  // Scroll bar when there's more than one page.
  if (lines_.size() > kVisibleLines) {
    const int top = kHeaderHeight + 2;
    const int track = 64 - top;
    const int thumb = std::max<int>(4, track * static_cast<int>(kVisibleLines) / static_cast<int>(lines_.size()));
    const int maxScroll = static_cast<int>(lines_.size() - kVisibleLines);
    const int y = top + (track - thumb) * static_cast<int>(scroll_) / maxScroll;
    oled_.drawBox(kWidth - 2, y, 2, thumb);
  }
}

// A boxed message in the middle of an otherwise empty screen.
void Hud::drawToast() {
  oled_.setFont(kFontText);
  const std::string msg = fit(toast_, kWidth - 16);
  const int w = textWidth(msg) + 14;
  const int h = 19;
  const int x = (kWidth - w) / 2;
  const int y = (64 - h) / 2;
  oled_.drawRFrame(x, y, w, h, 4);
  oled_.drawUTF8(x + 7, y + 13, msg.c_str());
}

void Hud::drawCentered(const std::string& s, int baseline) {
  oled_.drawUTF8((kWidth - textWidth(s)) / 2, baseline, s.c_str());
}

// Cuts a string to fit in maxWidth pixels (with the current font), adding "..".
std::string Hud::fit(const std::string& s, int maxWidth) {
  if (textWidth(s) <= maxWidth) return s;
  std::string out = s;
  while (!out.empty() && textWidth(out + "..") > maxWidth) {
    // Remove one whole UTF-8 character: continuation bytes, then its lead byte.
    while (!out.empty()) {
      const unsigned char c = static_cast<unsigned char>(out.back());
      out.pop_back();
      if ((c & 0xC0) != 0x80) break;
    }
  }
  return out + "..";
}

int Hud::textWidth(const std::string& s) { return oled_.getUTF8Width(s.c_str()); }
