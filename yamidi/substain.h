/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once
class CMG_Substain {
private:
  static constexpr int SUBSTAIN_RADIUS = 16;
  static constexpr int MAX_NOTE_VALUE = 80;
  //
  uint8_t p1 = 0;
  uint8_t p2 = 0;
  uint8_t p3 = 0;
  uint8_t prevP3 = 255;
public:
  static CMG_Substain& getInstance() {
    static CMG_Substain instance;
    return instance;
  }

  void drawSubstain() {
    // Pedals
    int radius = SUBSTAIN_RADIUS;
    int radiusR = SUBSTAIN_RADIUS - 1;
    if (p1 != 0) {
      CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_WHITE);
    } else {
      CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_GREY);
    }
    if (p2 != 0) {
      CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius + 2 * radius - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_WHITE);
    } else {
      CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius + 2 * radius - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_GREY);
    }
    int r = 2;
    if (p3 > 0) {
      uint8_t value = min(p3, (uint8_t)80);
      r = map(value, 0, MAX_NOTE_VALUE, 2, radiusR);
      if (p3 == 127) {
        // maximum circle size
        r = radiusR;
      }
      if (prevP3 != p3) {
        if (prevP3 > p3) {
          CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius + 2 * radius * 2 - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_GREY);
        }
        CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius + 2 * radius * 2 - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, r, CMG_Screen::CMG_WHITE);
        prevP3 = p3;
      }
    } else {
      CMG_Screen::getInstance().fillCircle(SCREEN_WIDTH / 2 + radius + 2 * radius * 2 - radius * 2 * 3 / 2, SCREEN_HEIGHT - radius - 4, radiusR, CMG_Screen::CMG_GREY);
    }
  }
};