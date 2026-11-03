#include "battery.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

namespace battery {

namespace {

bool g_ready = false;

#ifdef BATTERY_PIN

constexpr int kSamples = 16;  // averaged, to smooth out the ADC's noise

bool start() {
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);  // the pin sees up to about 2.1 V
  return true;
}

bool measure(Reading& out) {
  uint32_t sum = 0;
  for (int i = 0; i < kSamples; ++i) sum += analogReadMilliVolts(BATTERY_PIN);
  out.millivolts = static_cast<int>(sum * BATTERY_DIVIDER / kSamples);
  out.percent = -1;
  return true;
}

#else  // MAX17048 fuel gauge

constexpr uint8_t kAddress = 0x36;
constexpr uint8_t kVCell = 0x02;    // 78.125 microvolts per step
constexpr uint8_t kCharge = 0x04;   // 1/256 % per step
constexpr uint8_t kVersion = 0x08;  // 0x001_ on a MAX17048

bool readRegister(uint8_t reg, uint16_t& value) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(kAddress, static_cast<uint8_t>(2)) != 2) return false;
  const uint16_t high = Wire.read();
  value = static_cast<uint16_t>((high << 8) | Wire.read());
  return true;
}

bool start() {
  uint16_t version = 0;
  return readRegister(kVersion, version) && (version & 0xFFF0) == 0x0010;
}

bool measure(Reading& out) {
  uint16_t vcell = 0;
  uint16_t soc = 0;
  if (!readRegister(kVCell, vcell) || !readRegister(kCharge, soc)) return false;
  out.millivolts = static_cast<int>(vcell * 5UL / 64);  // 78.125 uV = 5/64 mV
  out.percent = soc >> 8;
  return true;
}

#endif

}  // namespace

bool begin() {
  g_ready = start();
  return g_ready;
}

bool read(Reading& out) { return g_ready && measure(out); }

}  // namespace battery
