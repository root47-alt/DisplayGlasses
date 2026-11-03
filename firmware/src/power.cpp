#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

namespace power {

bool canWakeFrom(int pin) { return pin >= 0 && rtc_gpio_is_valid_gpio(static_cast<gpio_num_t>(pin)); }

void afterWake(int pin) {
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 && canWakeFrom(pin)) {
    rtc_gpio_deinit(static_cast<gpio_num_t>(pin));
  }
}

void sleepUntilPressed(int pin) {
  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  // Hold the button's line high while asleep, so only a press pulls it low.
  // Input-only pins (GPIO 34-39 on the original ESP32, like the Feather V2's
  // button) have no pull-up of their own; the board provides one.
  if (GPIO_IS_VALID_OUTPUT_GPIO(gpio)) {
    rtc_gpio_pullup_en(gpio);
    rtc_gpio_pulldown_dis(gpio);
  }
  esp_sleep_enable_ext0_wakeup(gpio, 0);
  // Cut the power to the STEMMA QT port, where the screen plugs in. Starting
  // up turns it back on (the board's own setup does that).
#if defined(NEOPIXEL_I2C_POWER)
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);  // Feather V2
#elif defined(PIN_I2C_POWER)
  digitalWrite(PIN_I2C_POWER, LOW);  // ESP32-S3 Feather
#endif
  Serial.flush();
  esp_deep_sleep_start();
}

}  // namespace power
