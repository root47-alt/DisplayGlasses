#include "demo.h"

#include <Arduino.h>

#include "ancs.h"

namespace demo {
namespace {

struct Fake {
  const char* appId;
  uint8_t category;
  const char* title;
  const char* message;
};

// The same kinds of notifications the iPhone will send (see docs/iphone-setup.md).
const Fake kScript[] = {
    {"com.apple.shortcuts", ancs::kOther, "Weather", "24\xC2\xB0" "C Partly Cloudy"},
    {"com.apple.MobileSMS", ancs::kSocial, "Mom",
     "Are you coming home for dinner? I made enchiladas \xF0\x9F\x8C\xAE"},  // the emoji gets dropped
    {"com.apple.shortcuts", ancs::kOther, "Claude",
     "The Moon is about 384,400 km away. Its light takes about 1.3 seconds to reach your eyes, so "
     "you always see it slightly in the past."},
    {"com.apple.mobilecal", ancs::kSchedule, "Physics lab", "Starts in 15 minutes \xE2\x80\x93 Room 4102"},
    {"com.anthropic.claude", ancs::kOther, "Claude Code",
     "Your session finished: \xE2\x80\x9C" "Build the HUD firmware\xE2\x80\x9D. Tap to review."},
};
constexpr size_t kScriptLength = sizeof kScript / sizeof kScript[0];
constexpr uint32_t kFirstMs = 4000;
constexpr uint32_t kEveryMs = 14000;

NotifyFn g_notify;
size_t g_next = 0;
uint32_t g_dueAt = 0;

}  // namespace

void begin(clockutil::Clock& clock, const NotifyFn& notify) {
  g_notify = notify;
  clockutil::DateTime t;
  t.year = 2026;
  t.month = 11;
  t.day = 2;
  t.hour = 9;
  t.minute = 41;
  clock.set(t, millis());
  g_dueAt = millis() + kFirstMs;
}

void loop() {
  if (!g_notify || static_cast<int32_t>(millis() - g_dueAt) < 0) return;
  const Fake& f = kScript[g_next];
  g_notify(f.appId, f.category, f.title, "", f.message);
  g_next = (g_next + 1) % kScriptLength;
  g_dueAt = millis() + kEveryMs;
}

}  // namespace demo
