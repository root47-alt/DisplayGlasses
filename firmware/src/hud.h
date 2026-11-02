#pragma once
// Everything drawn on the 128x64 OLED: the home screen (clock, date,
// weather), notification pop-ups with auto-scroll, and short toasts.

#include <Arduino.h>
#include <U8g2lib.h>

#include <deque>
#include <string>
#include <vector>

#include "clock.h"
#include "notice.h"

// Light bouncing off the beam splitter flips the picture. Pick whichever mode
// makes text read correctly through your optics (long press cycles them).
enum class Mirror : uint8_t { None = 0, LeftRight, UpDown, Rotate180, Count };
const char* mirrorName(Mirror m);

// What the corner of the home screen says about the phone connection.
enum class Link : uint8_t { Demo, Searching, Pairing, Ready };

class Hud {
 public:
  // Starts I2C and finds the OLED at 0x3C or 0x3D. Returns false if no
  // screen answered.
  bool begin(int sda, int scl);

  void setMirror(Mirror m);
  Mirror mirror() const { return mirror_; }
  void setBrightness(uint8_t level);

  void setClock(const clockutil::Clock* clock, bool use24h);
  void setLink(Link link);
  void setWeather(const std::string& weather);

  // Shows a notification now, or after the ones already waiting.
  void push(const Notice& n);
  // A small message box over whatever is on screen.
  void toast(const std::string& message, uint32_t ms = 1500);
  // Clears the screen and shows two centered lines right away.
  void splash(const char* title, const char* detail);

  // Short button press: dismiss the current notification, or bring back the
  // last one when on the home screen.
  void onShortPress();

  // Call every loop(). Advances scrolling and timers, and redraws only when
  // something changed.
  void loop();

 private:
  struct Line {
    bool bold;
    std::string text;
  };

  void startNotice(const Notice& n, uint32_t now);
  void finishNotice(uint32_t now);
  void drawHome(uint32_t now);
  void drawNotice();
  void drawToast();
  void drawCentered(const std::string& s, int baseline);
  std::string fit(const std::string& s, int maxWidth);
  int textWidth(const std::string& s);

  U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled_{U8G2_R0, U8X8_PIN_NONE};
  bool present_ = false;
  Mirror mirror_ = Mirror::None;

  const clockutil::Clock* clock_ = nullptr;
  bool use24h_ = false;
  Link link_ = Link::Searching;
  std::string weather_;

  std::deque<Notice> queue_;
  Notice current_;
  Notice last_;
  bool showing_ = false;
  bool hasLast_ = false;
  std::vector<Line> lines_;
  size_t scroll_ = 0;
  uint32_t nextStepAt_ = 0;
  uint32_t endAt_ = 0;  // 0 = not decided yet (still scrolling)

  std::string toast_;
  uint32_t toastUntil_ = 0;

  bool dirty_ = true;
  int lastMinute_ = -1;
  uint32_t lastBlink_ = 0;
  bool blinkOn_ = true;
};
