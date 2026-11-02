#pragma once
// What the glasses show for an incoming iPhone notification. Pure C++.

#include <cstdint>
#include <string>

enum class NoticeKind : uint8_t {
  Generic,  // any app
  Claude,   // an answer from the "Ask Claude" shortcut
  Call,     // incoming phone call
};

struct Notice {
  uint32_t id = 0;    // the iPhone's id for it (0 = none), so it can be taken down again
  NoticeKind kind = NoticeKind::Generic;
  std::string app;    // header label, e.g. "Messages"
  std::string title;  // sender or headline, drawn in bold
  std::string body;   // the message itself
};

namespace notice {

// Friendly name for an iOS app id: "com.apple.MobileSMS" -> "Messages".
std::string appName(const std::string& bundleId);

enum class Route : uint8_t {
  Show,     // pop it up on the glasses
  Weather,  // update the weather line on the home screen
  Ignore,   // nothing to show
};

struct Routed {
  Route route = Route::Ignore;
  Notice notice;        // set for Route::Show
  std::string weather;  // set for Route::Weather
};

// Decides what to do with a notification. Notifications from the Shortcuts
// app titled "Weather..." become the home-screen weather line, and ones titled
// "Claude..." are shown as Claude answers (see docs/iphone-setup.md). All
// text comes back cleaned up with text::sanitize().
Routed route(const std::string& bundleId, uint8_t category, const std::string& title,
             const std::string& subtitle, const std::string& message);

}  // namespace notice
