/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once

class CMG_Spectre {
private:
  static constexpr int BANDS = 32;
  static constexpr int BANDS_HEIGHT = 64;
  static constexpr int MAX_NOTE_VALUE = 80;
  static constexpr int MAX_NB_NOTE = 128;
  //
  float bandHeight[BANDS];
  float bandVelocity[BANDS];
  float bandTarget[BANDS];
  uint8_t midiLevels[MAX_NB_NOTE] = { 0 };
  //
  void midiToTargets() {
    // reset targets
    for (int i = 0; i < BANDS; i++) {
      bandTarget[i] = 0;
    }
    for (int note = 0; note < MAX_NB_NOTE; note++) {
      int band = map(note, 0, MAX_NB_NOTE-1, 0, BANDS - 1);
      bandTarget[band] += midiLevels[note];
    }
    // clamp
    for (int i = 0; i < BANDS; i++) {
      bandTarget[i] = min((float)MAX_NOTE_VALUE, bandTarget[i]);
    }
  }

  void drawSpectrum(int posx, int posy) {
    int w = SCREEN_WIDTH / 4;
    int h = BANDS_HEIGHT;
    int barW = w / BANDS;
    for (int i = 0; i < BANDS; i++) {
      int value = (int)bandHeight[i];
      int barH = map(value, 0, MAX_NOTE_VALUE, 0, h);
      int x = i * barW;
      int y = h - barH;
      CMG_Screen::getInstance().fillRect(posx + x, posy, barW - 2, h - barH, CMG_Screen::CMG_GREY);
      CMG_Screen::getInstance().fillRect(posx + x, posy + y, barW - 2, barH, CMG_Screen::CMG_WHITE);
    }
  }
public:
  static CMG_Spectre& getInstance() {
    static CMG_Spectre instance;
    return instance;
  }

  void updateBands() {
    float stiffness = 0.15f;  // force du ressort
    float damping = 0.80f;    // perte énergie (rebond)
    float decay = 0.98f;      // disparition lente
    for (int i = 0; i < BANDS; i++) {
      float force = (bandTarget[i] - bandHeight[i]) * stiffness;
      bandVelocity[i] += force;
      bandVelocity[i] *= damping;
      bandHeight[i] += bandVelocity[i];
      // si plus de note → retour progressif à zéro
      bandTarget[i] *= decay;
      // sécurité
      if (bandHeight[i] < 0) bandHeight[i] = 0;
      if (bandHeight[i] > MAX_NOTE_VALUE) bandHeight[i] = MAX_NOTE_VALUE;
    }
  }

  void drawSpectrum() {
    drawSpectrum(SCREEN_WIDTH / 2 + BANDS_HEIGHT, SCREEN_HEIGHT / 2 - BANDS_HEIGHT);
  }
};