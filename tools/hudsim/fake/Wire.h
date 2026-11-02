#pragma once
// Fake I2C bus: an OLED "answers" at one address, like the real screen.
#include <cstdint>

class TwoWire {
 public:
  bool begin(int sda = -1, int scl = -1, uint32_t frequency = 0) {
    (void)sda;
    (void)scl;
    (void)frequency;
    return true;
  }
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t address) { address_ = address; }
  uint8_t endTransmission() { return address_ == screen_address ? 0 : 2; }

  uint8_t screen_address = 0x3C;

 private:
  uint8_t address_ = 0;
};

extern TwoWire Wire;
