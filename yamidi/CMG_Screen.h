/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once
//
#include <LovyanGFX.hpp>
#include "esp32-hal-psram.h"
#include "LGFX.h"

//
class CMG_Screen {
public:
  static constexpr int32_t CMG_YELLOW = 0x00FFFF00U;
  static constexpr int32_t CMG_GREEN = 0x0000FF00U;
  static constexpr int32_t CMG_WHITE = 0x00FFFFFFU;
  static constexpr int32_t CMG_NAVY = 0x000000FFU;
  static constexpr int32_t CMG_BLACK = 0x00000000U;
  static constexpr int32_t CMG_DARK_GREEN = 0x0000AA00U;
  static constexpr int32_t CMG_SKYBLUE = 0x0087CEEBU;
  static constexpr int32_t CMG_BLUE = 0x000000FFU;
  static constexpr int32_t CMG_GREY = 0x004F4F4FU;
  static constexpr int32_t CMG_RED = 0x00FF0000U;
  static constexpr int32_t CMG_WHITE_L1 = 0x00F0F0F0U;
  static constexpr int32_t CMG_WHITE_L2 = 0x00C0C0C0U;
  static constexpr int32_t CMG_WHITE_L3 = 0x00FFFFFFU;
private:
    inline static LGFX lcd{};

public:
    inline static LGFX_Sprite rollSprite{&lcd};

public:
  static CMG_Screen& getInstance() {
    static CMG_Screen instance;
    return instance;
  }

  LGFX_Sprite* getRollSprite(){
    return &rollSprite;
  }

  void setup() {
    lcd.init();
    // Setting display to landscape
    if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);
    lcd.setBrightness(128);
    lcd.setColorDepth(24);
    lcd.setTextFont(&fonts::Font6);
    rollSprite.setPsram(true);
    rollSprite.setColorDepth(24);
    rollSprite.createSprite(200, 80 * 2);
  }

  void setBrightness(uint8_t _brightness) {
    lcd.setBrightness(_brightness);
  }

  void fillScreen(int32_t _color) {
    lcd.fillScreen((uint32_t)_color);
  }

  void initScreen() {
    setup();
    fillScreen(CMG_BLACK);
    setBrightness(128);
  }

  void drawString(const String& text, uint16_t _x, uint16_t _y, uint8_t _size, uint32_t _color, uint32_t _backColor = TFT_BLACK) {
    lcd.setTextSize(1);
    if (_size == 1) {
      lcd.setTextFont(&fonts::Font0);
    } else if (_size == 2) {
      lcd.setTextFont(&fonts::Font2);
      lcd.setTextSize(2);  // x2
    } else if (_size == 4) {
      lcd.setTextFont(&fonts::Font4);
    }
    lcd.setTextColor(_color, _backColor);
    lcd.drawString(text.c_str(), _x, _y);
  }

  bool getTouch(int32_t* x, int32_t* y) {
    return lcd.getTouch(x, y);
  }

  bool getClickTouch(int32_t* x, int32_t* y) {
    bool result = lcd.getTouch(x, y);
    if (result) {
      while (result) {
        result = lcd.getTouch(x, y);
        delay(10);
      }
      return true;
    } else {
      return false;
    }
  }

  void drawRect(int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _color) {
    lcd.drawRect(_x, _y, _w, _h, (uint32_t)_color);
  }

  void fillRect(int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _color) {
    lcd.fillRect(_x, _y, _w, _h, (uint32_t)_color);
  }

  void fillSpriteRect(int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _color) {
    rollSprite.fillRect(_x, _y, _w, _h, (uint32_t)_color);
  }

  void fillRoundRect(int32_t _x, int32_t _y, int32_t _w, int32_t _h, int16_t _radius, int32_t _color) {
    lcd.fillRoundRect(_x, _y, _w, _h, _radius, (uint32_t)_color);
  }


  void drawRoundRect(int32_t _x, int32_t _y, int32_t _w, int32_t _h, int16_t _radius, int32_t _color) {
    lcd.drawRoundRect(_x, _y, _w, _h, _radius, (uint32_t)_color);
  }

  void drawLine(int32_t _x0, int32_t _y0, int32_t _x1, int32_t _y1, int32_t _color) {
    lcd.drawLine(_x0, _y0, _x1, _y1, (uint32_t)_color);
  }

  void fillCircle(int32_t _x0, int32_t _y0, int32_t _r, int32_t _color) {
    lcd.fillCircle(_x0, _y0, _r, (uint32_t)_color);
  }

  void drawCircle(int32_t _x0, int32_t _y0, int32_t _r, int32_t _color) {
    lcd.drawCircle(_x0, _y0, _r, (uint32_t)_color);
  }

  void drawBitmap(int32_t _x0, int32_t _y0, const uint32_t* data, int32_t _w, int32_t _h) {
    uint16_t index = 0;
    for (uint16_t y = 0; y < _h; y++) {
      for (uint16_t x = 0; x < _w; x++) {
        uint32_t color = data[index++];
        lcd.drawPixel(_x0 + x, _y0 + y, color);
      }
    }
  }
  void drawTitle(String title) {
    drawString(title.c_str(), SCREEN_WIDTH / 2 - 8 * title.length() / 2, 0, 1, CMG_WHITE);
    fillRect(0, 10, SCREEN_WIDTH, 2, CMG_RED);
  }
};