#pragma once
// Everything drawn on the 128x64 OLED: the home screen (clock, date,
// weather), notification pop-ups with auto-scroll, and short messages.

#include <Arduino.h>
#include <U8g2lib.h>

#include <deque>
#include <string>
#include <vector>

#include "clock.h"
#include "notice.h"

// Light bouncing off the beam splitter flips the picture. Pick whichever mode
// makes text read correctly through your optics (holding the button about
// 5 seconds cycles them).
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
  // Battery charge in percent for the corner of the home screen, or -1 to
  // hide it (no reading).
  void setBattery(int percent);

  // Shows a notification now, or after the ones already waiting. Calls jump
  // the line.
  void push(const Notice& n);
  // Takes a notification down (on screen or waiting), e.g. when it's read on
  // the phone or a call ends. Ignores ids it doesn't have.
  void remove(uint32_t id);
  // A short message that takes over the screen for a moment.
  void toast(const std::string& message, uint32_t ms = 1500);
  // Clears the screen and shows two centered lines right away.
  void splash(const char* title, const char* detail);
  // Two centered lines that stay on screen, over everything else, until
  // clearPrompt(). Shown while the button is held: "Turn off" / "let go now".
  void prompt(const std::string& title, const std::string& detail);
  void clearPrompt();
  // Turns the screen off, before the glasses go to sleep.
  void sleep();
  // Redraw on the next loop(), e.g. after the time was corrected.
  void refresh() { dirty_ = true; }

  // Short button press: dismiss the current notification, or bring back the
  // last one when on the home screen.
  void onShortPress();

  bool showing() const { return showing_; }
  size_t waiting() const { return queue_.size(); }

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
  void drawTwoLines(const std::string& title, const std::string& detail);
  int drawBattery(int right, int top);
  void drawCentered(const std::string& s, int baseline);
  std::string fit(const std::string& s, int maxWidth);
  int textWidth(const std::string& s);

  U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled_{U8G2_R0, U8X8_PIN_NONE};
  bool present_ = false;
  Mirror mirror_ = Mirror::None;

  const clockutil::Clock* clock_ = nullptr;
  bool use24h_ = false;
  int64_t lastClockKey_ = -1;  // the minute last drawn
  Link link_ = Link::Searching;
  std::string weather_;
  int battery_ = -1;

  std::deque<Notice> queue_;
  Notice current_;
  Notice last_;
  bool showing_ = false;
  bool hasLast_ = false;
  std::vector<Line> lines_;
  size_t scroll_ = 0;
  uint32_t nextStepAt_ = 0;
  uint32_t endAt_ = 0;
  bool endSet_ = false;  // false while the text is still scrolling

  std::string toast_;
  uint32_t toastUntil_ = 0;
  bool toastActive_ = false;

  std::string promptTitle_;
  std::string promptDetail_;
  bool prompting_ = false;

  bool dirty_ = true;
};
