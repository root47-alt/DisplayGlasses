#pragma once
// Just enough of Arduino.h to run the HUD code on a computer. Time is
// virtual: the simulator moves sim::now_ms forward itself.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace sim {
extern uint32_t now_ms;
}

inline uint32_t millis() { return sim::now_ms; }
inline void delay(uint32_t ms) { sim::now_ms += ms; }
