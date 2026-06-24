/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */

#pragma once
static constexpr int NB_NOTE_OCTAVE = 12;  // BAUD
class CMG_Helper {
private:
  static constexpr int SERIAL_SPEED = 115200;  // BAUD

public:
  static CMG_Helper& getInstance() {
    static CMG_Helper instance;
    return instance;
  }

  void initSerial() {
    Serial.begin(SERIAL_SPEED);
    while (!Serial) {
      ;
    }
  }

  String fill(String s, int length) {
    while (s.length() < length) {
      s += ' ';
    }
    return s.substring(0, length);
  }
};
