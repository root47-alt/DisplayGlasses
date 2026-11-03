// Runs the real HUD code (firmware/src/hud.cpp and lib/core) on a computer
// and records every frame it sends to the screen.
//
// Usage: hudsim <frames.jsonl>
// Each line: {"scene": "...", "t": <virtual ms>, "frame": "<1024 bytes as hex>"}
// render.py turns the file into PNGs and an animated GIF.

#include <Wire.h>

#include <cstdio>
#include <string>

#include "clock.h"
#include "demo.h"
#include "hud.h"
#include "notice.h"

namespace sim {
uint32_t now_ms = 1000;
uint8_t frame[1024];
uint32_t frames_sent = 0;
uint8_t power_save = 0;
}  // namespace sim

TwoWire Wire;

namespace {

FILE* g_out = nullptr;
std::string g_scene;
uint32_t g_lastSent = 0;

void capture(bool force = false) {
  if (!force && sim::frames_sent == g_lastSent) return;
  g_lastSent = sim::frames_sent;
  std::fprintf(g_out, "{\"scene\":\"%s\",\"t\":%u,\"frame\":\"", g_scene.c_str(), sim::now_ms);
  for (uint8_t b : sim::frame) std::fprintf(g_out, "%02x", b);
  std::fprintf(g_out, "\"}\n");
}

// Runs the HUD for `ms` of virtual time, recording each new frame.
void run(Hud& hud, uint32_t ms, void (*tick)() = nullptr) {
  const uint32_t end = sim::now_ms + ms;
  while (sim::now_ms < end) {
    if (tick) tick();
    hud.loop();
    capture();
    sim::now_ms += 20;
  }
}

// Same routing as main.cpp.
Hud* g_hud = nullptr;
void onNotification(uint32_t uid, const std::string& appId, uint8_t category, const std::string& title,
                    const std::string& subtitle, const std::string& message) {
  notice::Routed r = notice::route(appId, category, title, subtitle, message);
  r.notice.id = uid;
  if (r.route == notice::Route::Show) g_hud->push(r.notice);
  if (r.route == notice::Route::Weather) g_hud->setWeather(r.weather);
}

clockutil::DateTime nineFortyOne() {
  clockutil::DateTime t;
  t.year = 2026;
  t.month = 11;
  t.day = 2;
  t.hour = 9;
  t.minute = 41;
  return t;
}

void scene(const char* name) {
  g_scene = name;
  sim::now_ms += 5000;  // a gap between scenes
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s frames.jsonl\n", argv[0]);
    return 2;
  }
  g_out = std::fopen(argv[1], "w");
  if (!g_out) return 1;

  clockutil::Clock phone;
  clockutil::Clock noTime;

  {
    scene("splash");
    Hud hud;
    hud.begin(0, 0);
    hud.splash("DisplayGlasses", "pair in Bluetooth");
    capture(true);
  }
  {
    scene("home_no_phone");
    Hud hud;
    hud.begin(0, 0);
    hud.setClock(&noTime, false);
    hud.setLink(Link::Searching);
    hud.setBattery(55);
    run(hud, 2500);
  }
  {
    scene("home");
    Hud hud;
    g_hud = &hud;
    hud.begin(0, 0);
    phone.set(nineFortyOne(), sim::now_ms);
    hud.setClock(&phone, false);
    hud.setLink(Link::Ready);
    hud.setBattery(80);
    onNotification(1, "com.apple.shortcuts", 0, "Weather", "", "24\xC2\xB0" "C Partly Cloudy");
    run(hud, 200);

    scene("message");
    onNotification(2, "com.apple.MobileSMS", 4, "Mom", "",
                   "Are you coming home for dinner? I made enchiladas \xF0\x9F\x8C\xAE");
    run(hud, 12000);

    scene("claude");
    onNotification(3, "com.apple.shortcuts", 0, "Claude", "",
                   "The Moon is about 384,400 km away. Its light takes about 1.3 seconds to reach your "
                   "eyes, so you always see it slightly in the past.");
    run(hud, 30000);

    scene("call");
    onNotification(4, "com.apple.mobilephone", 1, "Mom", "", "Incoming Call");
    run(hud, 3000);
    hud.remove(4);  // the call was answered: the phone removes the notification
    run(hud, 200);

    scene("long_title");
    onNotification(5, "com.apple.mobilemail", 6, "Universidad \xE2\x80\x93 Oficina de Becas",
                   "Resultado de tu solicitud",
                   "Hola Jos\xC3\xA9, ya est\xC3\xA1 disponible el resultado de tu solicitud de beca.");
    run(hud, 30000);

    scene("mirror_toast");
    hud.setMirror(Mirror::LeftRight);
    hud.toast("Mirror: left-right");
    run(hud, 300);
    hud.setMirror(Mirror::None);
    run(hud, 2000);

    scene("hold_off");
    hud.prompt("Turn off", "let go now");
    run(hud, 300);
    scene("hold_flip");
    hud.prompt("Flip image", "let go now");
    run(hud, 300);
    hud.clearPrompt();
    run(hud, 200);
  }
  {
    scene("turning_off");
    Hud hud;
    hud.begin(0, 0);
    hud.splash("Turning off", "press to turn on");
    capture(true);
  }
  {
    scene("demo");
    Hud hud;
    g_hud = &hud;
    hud.begin(0, 0);
    hud.setClock(&phone, false);
    hud.setLink(Link::Demo);
    hud.setBattery(90);
    demo::begin(phone, onNotification);
    run(hud, 75000, [] { demo::loop(); });
  }

  std::fclose(g_out);
  std::printf("frames sent: %u\n", sim::frames_sent);
  return 0;
}
