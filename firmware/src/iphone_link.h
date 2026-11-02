#pragma once
// Bluetooth link to an iPhone. The glasses pair like a smartwatch and use two
// services built into iOS, so no iPhone app is needed:
//   - ANCS (Apple Notification Center Service): every notification on the phone
//   - CTS (Current Time Service): the phone's clock

#include <cstdint>
#include <functional>
#include <string>

#include "clock.h"

namespace iphone {

struct Handlers {
  // A new notification arrived (raw text, not yet cleaned up).
  std::function<void(const std::string& appId, uint8_t category, const std::string& title,
                     const std::string& subtitle, const std::string& message)>
      onNotification;
  // The phone told us its local time.
  std::function<void(const clockutil::DateTime& localTime)> onTime;
  // connected: a phone is connected. ready: paired and sharing notifications.
  std::function<void(bool connected, bool ready)> onLinkChange;
};

// Starts Bluetooth and advertises as `name` until an iPhone connects.
void begin(const char* name, const Handlers& handlers);

// Call every loop(). Slow Bluetooth work (service discovery, requests) runs
// here rather than inside the Bluetooth task.
void loop();

// Forgets the paired iPhone. Restart afterwards, then "Forget This Device" on
// the iPhone and pair again.
void forget();

}  // namespace iphone
