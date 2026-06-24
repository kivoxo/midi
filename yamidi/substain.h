/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once
class CMG_Substain {
private:
  static constexpr int SUBSTAIN_RADIUS = 16;
  static constexpr uint8_t MAX_NOTE_VALUE = 80;
  static constexpr uint8_t NB_SUBSTAIN = 3;
  uint8_t values[NB_SUBSTAIN];
  uint8_t prevP3 = 255;

public:
  static CMG_Substain& getInstance() {
    static CMG_Substain instance;
    return instance;
  }

  void setSubstain(uint8_t id, uint8_t value){
    values[id] = value;
  }

  void drawSubstain() {
    // Pedals
    int radius = SUBSTAIN_RADIUS;
    int radiusR = SUBSTAIN_RADIUS - 1;
     // 
    int posX = SCREEN_WIDTH / 2 + radius - radius * 2 * 3 / 2;
    int posY = SCREEN_HEIGHT - radius - 4;
    if (values[0] != 0) {
      CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_WHITE);
    } else {
      CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_GREY);
    }
    //
    posX += + 2 * radius;
    if (values[1] != 0) {
      CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_WHITE);
    } else {
      CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_GREY);
    }
    //
    int r = 2;
    posX += + 2 * radius;
    if (values[2] > 0) {
      uint8_t value = min(values[2], MAX_NOTE_VALUE);
      r = map(value, 0, MAX_NOTE_VALUE, 2, radiusR);
      if (values[2] == 127) {
        // maximum circle size
        r = radiusR;
      }
      if (prevP3 != values[2]) {
        if (prevP3 > values[2]) {
          CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_GREY);
        }
        CMG_Screen::getInstance().fillCircle(posX, posY, r, CMG_Screen::CMG_WHITE);
        prevP3 = values[2];
      }
    } else {
      CMG_Screen::getInstance().fillCircle(posX, posY, radiusR, CMG_Screen::CMG_GREY);
    }
  }
};