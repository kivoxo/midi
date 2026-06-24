/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once

#include "pianokey.h"

class CMG_Piano {
private:
  static constexpr int PIANO_START_KEY_SHIFT = 21;
  static constexpr int KEYBOARD_NB_KEY = 88;
  static constexpr int KEYBOARD_NB_WHITE_KEY = 52;
  static constexpr int KEYBOARD_X = 6;
  static constexpr int KEYBOARD_Y = 240;
  static constexpr int BLACK_W = 6;
  static constexpr int BLACK_H = 25;
  static constexpr int WHITE_W = 9;
  static constexpr int WHITE_H = 40;
  struct WhiteMask {
    bool cutLeft = false;
    bool cutRight = false;
  };
  WhiteMask whiteMask[KEYBOARD_NB_WHITE_KEY];
  CMG_PianoKey::Key keyboard[KEYBOARD_NB_KEY];
   static constexpr int MAX_KEY = 512;
  uint8_t keys[MAX_KEY];

private:
  void precomputeWhiteMask() {
    for (int i = 0; i < KEYBOARD_NB_WHITE_KEY; i++) {
      whiteMask[i].cutLeft = false;
      whiteMask[i].cutRight = false;
    }
    for (int i = 0; i < KEYBOARD_NB_KEY; i++) {
      if (keyboard[i].isBlack) {
        int l = keyboard[i].blackLeftWhite;
        int r = keyboard[i].blackRightWhite;
        if (l >= 0 && l < KEYBOARD_NB_WHITE_KEY) {
          whiteMask[l].cutRight = true;
        }
        if (r >= 0 && r < KEYBOARD_NB_WHITE_KEY) {
          whiteMask[r].cutLeft = true;
        }
      }
    }
  }

public:
  static CMG_Piano& getInstance() {
    static CMG_Piano instance;
    return instance;
  }
  void buildKeyboard() {
    int whiteIndex = 0;
    for (int i = 0; i < KEYBOARD_NB_KEY; i++) {
      int midi = i + PIANO_START_KEY_SHIFT;
      int p = midi % NB_NOTE_OCTAVE;
      bool black = (p == 1 || p == 3 || p == 6 || p == 8 || p == 10);
      keyboard[i].isBlack = black;
      if (!black) {
        keyboard[i].whiteIndex = whiteIndex;
        keyboard[i].blackLeftWhite = -1;
        keyboard[i].blackRightWhite = -1;
        whiteIndex++;
      } else {
        keyboard[i].whiteIndex = -1;
        keyboard[i].blackLeftWhite = whiteIndex - 1;
        keyboard[i].blackRightWhite = whiteIndex;
      }
    }
    precomputeWhiteMask();
  }

  void keyStatus(uint8_t midiNote, uint8_t keyStatus) {
    keys[midiNote] = keyStatus;
  }

  void drawPiano() {
    int shift = PIANO_START_KEY_SHIFT;
    int whiteKey = 0;
    // White keys
    for (int i = 0; i < KEYBOARD_NB_KEY; i++) {
      if (!keyboard[i].isBlack) {
        int w = keyboard[i].whiteIndex;
        int x = w * WHITE_W;
        bool pressed = keys[i + shift];
        int y = KEYBOARD_Y;
        int x2 = x;
        int w2 = WHITE_W - 1;
        // Upper section (cutout under black keys)
        int topH = WHITE_H * 0.6;  // area under black keys
        int topY = y;
        int bottomY = y + topH;
        int bottomH = WHITE_H - topH;
        if (whiteMask[w].cutLeft) {
          x2 += BLACK_W / 2;
          w2 -= BLACK_W / 2;
        }
        if (whiteMask[w].cutRight) {
          w2 -= BLACK_W / 2;
        }
        // TOP (with cutout)
        CMG_Screen::getInstance().fillRect(KEYBOARD_X + x2, topY, w2, topH, !pressed ? CMG_Screen::CMG_WHITE : CMG_Screen::CMG_GREY);
        // BASE (full width, always complete)
        CMG_Screen::getInstance().fillRect(KEYBOARD_X + x, bottomY, WHITE_W - 1, bottomH, !pressed ? CMG_Screen::CMG_WHITE : CMG_Screen::CMG_GREY);
      }
    }
    // Black
    for (int i = 0; i < KEYBOARD_NB_KEY; i++) {
      if (keyboard[i].isBlack) {
        int l = keyboard[i].blackLeftWhite;
        int r = keyboard[i].blackRightWhite;
        if (l < 0 || r < 0) continue;
        // Exact position between two white keys
        int x = l * WHITE_W + (WHITE_W - BLACK_W / 2);
        bool pressed = keys[i + shift];
        CMG_Screen::getInstance().fillRoundRect(KEYBOARD_X + x, KEYBOARD_Y, BLACK_W, BLACK_H, 1, !pressed ? CMG_Screen::CMG_BLACK : CMG_Screen::CMG_BLUE);
      }
    }
    CMG_Screen::getInstance().fillRect(0, KEYBOARD_Y - 2, SCREEN_WIDTH, 2, CMG_Screen::CMG_RED);
  }
};