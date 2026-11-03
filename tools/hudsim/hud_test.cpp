// Behavior tests for the screen code (firmware/src/hud.cpp), run on a computer
// through the same stand-ins as the simulator. They check what's actually
// drawn, pixel by pixel.
//
// Built and run by tools/hudsim/build.sh --test (and by CI).

#include <Wire.h>

#include <cstdio>
#include <vector>

#include "clock.h"
#include "hud.h"

namespace sim {
uint32_t now_ms = 1000;
uint8_t frame[1024];
uint32_t frames_sent = 0;
uint8_t power_save = 0;
}  // namespace sim

TwoWire Wire;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    ++g_checks;                                                              \
    if (!(cond)) {                                                           \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

void advance(Hud& hud, uint32_t ms) {
  const uint32_t end = sim::now_ms + ms;
  while (static_cast<int32_t>(sim::now_ms - end) < 0) {
    hud.loop();
    sim::now_ms += 10;
  }
  hud.loop();
}

bool pixel(const std::vector<uint8_t>& f, int x, int y) { return (f[(y / 8) * 128 + x] >> (y % 8)) & 1; }
std::vector<uint8_t> snapshot() { return std::vector<uint8_t>(sim::frame, sim::frame + 1024); }

int litIn(const std::vector<uint8_t>& f, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y <= y1; ++y)
    for (int x = x0; x <= x1; ++x) n += pixel(f, x, y);
  return n;
}

Notice make(uint32_t id, NoticeKind kind, const char* title, const char* body) {
  Notice n;
  n.id = id;
  n.kind = kind;
  n.app = kind == NoticeKind::Call ? "Call" : (kind == NoticeKind::Claude ? "Claude" : "Messages");
  n.title = title;
  n.body = body;
  return n;
}

clockutil::DateTime at(int hour, int minute, int second = 0) {
  clockutil::DateTime t;
  t.year = 2026;
  t.month = 11;
  t.day = 2;
  t.hour = hour;
  t.minute = minute;
  t.second = second;
  return t;
}

const char* kLong =
    "The Moon is about 384,400 km away. Its light takes about 1.3 seconds to reach your eyes, so you "
    "always see it slightly in the past.";

// ------------------------------------------------------------------ tests

void call_disappears_when_it_ends() {
  Hud hud;
  hud.begin(0, 0);
  hud.push(make(7, NoticeKind::Call, "Mom", "Incoming Call"));
  advance(hud, 2000);
  CHECK(hud.showing());
  hud.remove(7);
  advance(hud, 50);
  CHECK(!hud.showing());
}

void newer_call_replaces_older_call() {
  Hud hud;
  hud.begin(0, 0);
  hud.push(make(1, NoticeKind::Generic, "Ana", "hi"));
  hud.push(make(2, NoticeKind::Call, "Mom", "Incoming Call"));
  hud.push(make(3, NoticeKind::Call, "Dad", "Incoming Call"));
  CHECK(hud.waiting() == 1);  // Ana's message waits; Mom's call was replaced, not queued
  hud.remove(3);
  advance(hud, 50);
  CHECK(hud.showing());  // back to Ana's message
  CHECK(hud.waiting() == 0);
}

void queue_keeps_the_newest_eight() {
  Hud hud;
  hud.begin(0, 0);
  for (uint32_t i = 1; i <= 12; ++i) hud.push(make(i, NoticeKind::Generic, "x", "y"));
  CHECK(hud.showing());
  CHECK(hud.waiting() == 8);
}

void removed_while_waiting() {
  Hud hud;
  hud.begin(0, 0);
  hud.push(make(1, NoticeKind::Generic, "a", "b"));
  hud.push(make(2, NoticeKind::Generic, "c", "d"));
  hud.push(make(3, NoticeKind::Generic, "e", "f"));
  hud.remove(2);
  CHECK(hud.waiting() == 1);
  hud.remove(99);  // unknown ids are ignored
  CHECK(hud.waiting() == 1);
}

void long_text_scrolls_then_closes() {
  Hud hud;
  hud.begin(0, 0);
  hud.push(make(1, NoticeKind::Claude, "", kLong));
  advance(hud, 100);
  const std::vector<uint8_t> first = snapshot();
  advance(hud, 6000);
  CHECK(snapshot() != first);  // it has scrolled
  CHECK(hud.showing());
  advance(hud, 40000);
  CHECK(!hud.showing());       // and closed by itself
}

void toast_takes_over_then_clears() {
  clockutil::Clock clock;
  clock.set(at(9, 41), sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Ready);
  advance(hud, 100);
  const std::vector<uint8_t> home = snapshot();
  hud.toast("Mirror: none", 1500);
  advance(hud, 100);
  const std::vector<uint8_t> toast = snapshot();
  CHECK(toast != home);
  CHECK(litIn(toast, 0, 0, 127, 15) == 0);  // nothing of the clock or date peeks out above the box
  advance(hud, 1600);
  CHECK(snapshot() == home);
}

void prompt_stays_until_cleared() {
  clockutil::Clock clock;
  clock.set(at(9, 41), sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Ready);
  advance(hud, 100);
  const std::vector<uint8_t> home = snapshot();
  hud.toast("Mirror: none", 1500);
  hud.prompt("Turn off", "let go now");
  advance(hud, 100);
  const std::vector<uint8_t> shown = snapshot();
  CHECK(shown != home);
  CHECK(litIn(shown, 0, 0, 127, 15) == 0);   // nothing else on screen: no clock, no toast box
  CHECK(litIn(shown, 0, 19, 127, 29) > 20);  // the title
  CHECK(litIn(shown, 0, 35, 127, 45) > 20);  // the detail line
  advance(hud, 5000);                        // outlasts the toast
  CHECK(snapshot() == shown);
  hud.clearPrompt();
  advance(hud, 100);
  CHECK(snapshot() == home);
}

void battery_shows_in_the_corner() {
  clockutil::Clock clock;
  clock.set(at(9, 40), sim::now_ms);  // a minute when the burn-in shift is zero
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Ready);
  advance(hud, 100);
  CHECK(litIn(snapshot(), 80, 0, 127, 10) == 0);  // no reading: nothing in the corner
  hud.setBattery(100);
  advance(hud, 100);
  const std::vector<uint8_t> full = snapshot();
  CHECK(litIn(full, 112, 2, 125, 8) > 40);  // the battery
  CHECK(litIn(full, 80, 2, 108, 9) > 10);   // "100%"
  CHECK(litIn(full, 114, 4, 121, 6) == 24);  // filled all the way
  hud.setBattery(0);
  advance(hud, 100);
  CHECK(litIn(snapshot(), 114, 4, 121, 6) == 0);  // empty
  hud.setBattery(50);
  advance(hud, 100);
  CHECK(litIn(snapshot(), 114, 4, 121, 6) == 12);  // half
}

void corner_stays_clear_of_the_date() {
  // The longest date with the longest status and the battery.
  clockutil::DateTime t = at(9, 40);
  t.month = 9;
  t.day = 30;
  clockutil::Clock clock;
  clock.set(t, sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Searching);
  hud.setBattery(50);
  advance(hud, 100);
  const std::vector<uint8_t> f = snapshot();
  bool gap = false;
  for (int x = 58; x <= 72 && !gap; ++x) gap = litIn(f, x, 0, x + 1, 10) == 0;
  CHECK(gap);  // two blank columns somewhere between the date and "no phone"
  CHECK(litIn(f, 2, 0, 50, 10) > 30);    // the date
  CHECK(litIn(f, 70, 0, 108, 10) > 30);  // "no phone"
}

void sleep_turns_the_screen_off() {
  Hud hud;
  hud.begin(0, 0);
  CHECK(sim::power_save == 0);
  hud.sleep();
  CHECK(sim::power_save == 1);
}

void splash_text_fits_on_screen() {
  Hud hud;
  hud.begin(0, 0);
  hud.splash("DisplayGlasses", "pair in Bluetooth");
  const std::vector<uint8_t> f = snapshot();
  CHECK(litIn(f, 0, 0, 1, 63) == 0);
  CHECK(litIn(f, 126, 0, 127, 63) == 0);
  hud.splash("DisplayGlasses", "a very long line that would never fit on this little screen");
  const std::vector<uint8_t> g = snapshot();
  CHECK(litIn(g, 0, 0, 1, 63) == 0);  // long text is shortened, not clipped
  CHECK(litIn(g, 126, 0, 127, 63) == 0);
}

void mirror_left_right_is_an_exact_mirror() {
  clockutil::Clock clock;
  clock.set(at(9, 41), sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Ready);
  advance(hud, 100);
  const std::vector<uint8_t> plain = snapshot();
  hud.setMirror(Mirror::LeftRight);
  advance(hud, 100);
  const std::vector<uint8_t> mirrored = snapshot();
  int mismatches = 0;
  for (int y = 0; y < 64; ++y)
    for (int x = 0; x < 128; ++x) mismatches += pixel(plain, x, y) != pixel(mirrored, 127 - x, y);
  CHECK(mismatches == 0);
}

void clock_redraws_when_the_hour_changes() {
  clockutil::Clock clock;
  clock.set(at(10, 15), sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.setLink(Link::Ready);
  advance(hud, 100);
  const std::vector<uint8_t> before = snapshot();
  clock.set(at(11, 15), sim::now_ms);  // a time-zone change: same minute, new hour
  advance(hud, 100);
  CHECK(snapshot() != before);
}

void header_clock_updates_during_a_notice() {
  clockutil::Clock clock;
  clock.set(at(9, 41, 58), sim::now_ms);
  Hud hud;
  hud.begin(0, 0);
  hud.setClock(&clock, false);
  hud.push(make(1, NoticeKind::Generic, "Mom", "On my way"));
  advance(hud, 500);
  const uint32_t sent = sim::frames_sent;
  advance(hud, 2500);  // 9:42 arrives while the message is up
  CHECK(hud.showing());
  CHECK(sim::frames_sent > sent);
}

void status_shows_right_away_without_a_phone() {
  Hud hud;
  hud.begin(0, 0);
  hud.setLink(Link::Searching);
  advance(hud, 50);
  CHECK(litIn(snapshot(), 64, 0, 127, 10) > 0);  // "no phone" in the top right
}

}  // namespace

int main() {
  struct Test {
    const char* name;
    void (*fn)();
  };
  const Test tests[] = {
      {"call disappears when it ends", call_disappears_when_it_ends},
      {"newer call replaces older call", newer_call_replaces_older_call},
      {"queue keeps the newest eight", queue_keeps_the_newest_eight},
      {"removed while waiting", removed_while_waiting},
      {"long text scrolls then closes", long_text_scrolls_then_closes},
      {"toast takes over then clears", toast_takes_over_then_clears},
      {"prompt stays until cleared", prompt_stays_until_cleared},
      {"sleep turns the screen off", sleep_turns_the_screen_off},
      {"battery shows in the corner", battery_shows_in_the_corner},
      {"corner stays clear of the date", corner_stays_clear_of_the_date},
      {"splash text fits on screen", splash_text_fits_on_screen},
      {"mirror left-right is an exact mirror", mirror_left_right_is_an_exact_mirror},
      {"clock redraws when the hour changes", clock_redraws_when_the_hour_changes},
      {"header clock updates during a notice", header_clock_updates_during_a_notice},
      {"status shows right away without a phone", status_shows_right_away_without_a_phone},
  };
  for (const Test& t : tests) {
    const int before = g_failures;
    t.fn();
    std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", t.name);
  }
  std::printf("%d checks, %d failed\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
