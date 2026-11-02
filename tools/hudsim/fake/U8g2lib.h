#pragma once
// Stand-in for U8g2's Arduino class. Drawing goes through the real U8g2 C
// library (same fonts, same pixels as on the glasses), but nothing is sent
// over I2C: sendBuffer() copies the frame into sim::frame instead.
#include <cstdint>
#include <cstring>

#include "clib/u8g2.h"

namespace sim {
extern uint8_t frame[1024];   // 8 pages x 128 columns; bit 0 of each byte is the top pixel
extern uint32_t frames_sent;
}

class U8G2 {
 public:
  void setI2CAddress(uint8_t address) { u8g2_SetI2CAddress(&u8g2, address); }
  void setBusClock(uint32_t) {}
  bool begin() {
    u8g2_InitDisplay(&u8g2);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    return true;
  }
  void setDisplayRotation(const u8g2_cb_t* cb) { u8g2_SetDisplayRotation(&u8g2, cb); }
  void setContrast(uint8_t value) { u8g2_SetContrast(&u8g2, value); }
  void clearBuffer() { u8g2_ClearBuffer(&u8g2); }
  void sendBuffer() {
    u8g2_SendBuffer(&u8g2);
    std::memcpy(sim::frame, u8g2_GetBufferPtr(&u8g2), sizeof sim::frame);
    ++sim::frames_sent;
  }
  void setFont(const uint8_t* font) { u8g2_SetFont(&u8g2, font); }
  u8g2_uint_t drawUTF8(u8g2_uint_t x, u8g2_uint_t y, const char* s) { return u8g2_DrawUTF8(&u8g2, x, y, s); }
  u8g2_uint_t getUTF8Width(const char* s) { return u8g2_GetUTF8Width(&u8g2, s); }
  void drawBox(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h) { u8g2_DrawBox(&u8g2, x, y, w, h); }
  void setDrawColor(uint8_t color) { u8g2_SetDrawColor(&u8g2, color); }
  void drawRFrame(u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h, u8g2_uint_t r) {
    u8g2_DrawRFrame(&u8g2, x, y, w, h, r);
  }

 protected:
  u8g2_t u8g2;
};

class U8G2_SSD1306_128X64_NONAME_F_HW_I2C : public U8G2 {
 public:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C(const u8g2_cb_t* rotation, uint8_t reset = U8X8_PIN_NONE) {
    (void)reset;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, rotation, u8x8_byte_empty, u8x8_dummy_cb);
  }
};
