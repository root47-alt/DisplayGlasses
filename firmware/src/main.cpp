// DisplayGlasses: a heads-up display for one eye that shows iPhone
// notifications, the time, the weather and answers from Claude.
//
// See ../README.md for how it fits together.

#include <Arduino.h>
#include <Preferences.h>

#include "battery.h"
#include "button.h"
#include "charge.h"
#include "clock.h"
#include "config.h"
#include "hud.h"
#include "notice.h"
#include "power.h"

#ifdef DEMO_MODE
#include "demo.h"
#else
#include "iphone_link.h"
#endif

namespace {

Hud hud;
Button button(BUTTON_PIN);
clockutil::Clock phoneClock;
Preferences prefs;
charge::Gauge gauge;
bool haveBattery = false;
uint32_t nextBatteryAt = 0;
constexpr uint32_t kBatteryEveryMs = 5000;

void onNotification(uint32_t uid, const std::string& appId, uint8_t category, const std::string& title,
                    const std::string& subtitle, const std::string& message) {
  Serial.printf("[notify] %s: %s\n", appId.c_str(), title.c_str());
  notice::Routed r = notice::route(appId, category, title, subtitle, message);
  r.notice.id = uid;
  switch (r.route) {
    case notice::Route::Show: hud.push(r.notice); break;
    case notice::Route::Weather: hud.setWeather(r.weather); break;
    case notice::Route::Ignore: break;
  }
}

void cycleMirror() {
  const uint8_t count = static_cast<uint8_t>(Mirror::Count);
  const Mirror next = static_cast<Mirror>((static_cast<uint8_t>(hud.mirror()) + 1) % count);
  hud.setMirror(next);
  prefs.putUChar("mirror", static_cast<uint8_t>(next));
  hud.toast(std::string("Mirror: ") + mirrorName(next));
}

// Deep sleep until the button is pressed again, which starts the glasses up
// from the beginning (see power.h).
void turnOff(const char* title = "Turning off", const char* detail = "press to turn on") {
  if (!power::canWakeFrom(BUTTON_PIN)) {
    hud.toast("Can't turn off");  // the button's pin can't wake the chip
    return;
  }
  Serial.println(title);
  hud.splash(title, detail);
  delay(1500);
  hud.sleep();
  power::sleepUntilPressed(BUTTON_PIN);
}

// Every few seconds: update the battery level, warn when it gets low, and
// turn off when it's empty.
void checkBattery() {
  if (!haveBattery) return;
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextBatteryAt) < 0) return;
  nextBatteryAt = now + kBatteryEveryMs;
  battery::Reading r;
  if (!battery::read(r)) return;
  const int before = gauge.percent();
  const charge::Event event = gauge.add(now, r.millivolts, r.percent);
  if (gauge.percent() != before) Serial.printf("[battery] %d mV, %d%%\n", r.millivolts, gauge.percent());
  hud.setBattery(gauge.percent());
  switch (event) {
    case charge::Event::Low: hud.toast("Battery low: " + std::to_string(gauge.percent()) + "%", 4000); break;
    case charge::Event::Empty: turnOff("Battery empty", "charge, then press"); break;
    case charge::Event::None: break;
  }
}

void forgetPhone() {
#ifdef DEMO_MODE
  hud.toast("Demo: no iPhone");
#else
  hud.splash("Forgot iPhone", "restarting...");
  iphone::forget();
  delay(1500);
  ESP.restart();
#endif
}

}  // namespace

void setup() {
  Serial.begin(115200);
  power::afterWake(BUTTON_PIN);
#ifdef NEOPIXEL_POWER
  digitalWrite(NEOPIXEL_POWER, LOW);  // the Feather's RGB LED isn't used; save the power
#endif
  button.begin();
  prefs.begin("glasses", false);

  if (!hud.begin(I2C_SDA, I2C_SCL)) {
    Serial.println("No OLED found at I2C address 0x3C or 0x3D. Check the screen cable.");
  }
  hud.setMirror(static_cast<Mirror>(prefs.getUChar("mirror", DEFAULT_MIRROR) % static_cast<uint8_t>(Mirror::Count)));
  hud.setBrightness(SCREEN_BRIGHTNESS);
  hud.setClock(&phoneClock, USE_24H_CLOCK);
  haveBattery = battery::begin();
  checkBattery();

#ifdef DEMO_MODE
  hud.setLink(Link::Demo);
  hud.splash("DisplayGlasses", "demo mode");
  delay(1500);
  demo::begin(phoneClock, onNotification);
#else
  hud.setLink(Link::Searching);
  hud.splash("DisplayGlasses", "pair in Bluetooth");
  delay(1500);
  iphone::Handlers handlers;
  handlers.onNotification = onNotification;
  handlers.onRemoved = [](uint32_t uid) { hud.remove(uid); };
  handlers.onTime = [](const clockutil::DateTime& t) {
    phoneClock.set(t, millis());
    hud.refresh();
  };
  handlers.onLinkChange = [](bool connected, bool ready) {
    static bool wasReady = false;
    hud.setLink(ready ? Link::Ready : (connected ? Link::Pairing : Link::Searching));
    // Only announce changes from a working connection, so a phone that keeps
    // retrying doesn't flash messages.
    if (ready && !wasReady) hud.toast("iPhone connected");
    else if (!ready && wasReady) hud.toast(connected ? "Reconnecting..." : "iPhone disconnected");
    wasReady = ready;
  };
  iphone::begin(DEVICE_NAME, handlers);
#endif
}

void loop() {
  switch (button.update()) {
    case Button::Event::Short:
      hud.onShortPress();
      break;
    case Button::Event::ShowOff:
      hud.prompt("Turn off", "let go now");
      break;
    case Button::Event::Off:
      hud.clearPrompt();
      turnOff();
      break;
    case Button::Event::ShowFlip:
      hud.prompt("Flip image", "let go now");
      break;
    case Button::Event::Flip:
      hud.clearPrompt();
      cycleMirror();
      break;
    case Button::Event::Forget:
      hud.clearPrompt();
      forgetPhone();
      break;
    case Button::Event::None:
      break;
  }

  checkBattery();
#ifdef DEMO_MODE
  demo::loop();
#else
  iphone::loop();
#endif
  hud.loop();
  delay(5);
}
