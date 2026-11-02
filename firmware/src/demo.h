#pragma once
// Demo mode: fake notifications on a timer, so you can test the screen, the
// optics and the Wokwi simulator without an iPhone.

#include <cstdint>
#include <functional>
#include <string>

#include "clock.h"

namespace demo {

using NotifyFn = std::function<void(uint32_t uid, const std::string& appId, uint8_t category,
                                    const std::string& title, const std::string& subtitle,
                                    const std::string& message)>;

// Sets the clock to 9:41 and starts the fake notifications.
void begin(clockutil::Clock& clock, const NotifyFn& notify);
void loop();

}  // namespace demo
