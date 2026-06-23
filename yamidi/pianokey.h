/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once
class CMG_PianoKey {
private:
  static constexpr int MIDI_HISTORY_SIZE = 1024;
  static constexpr int ROLL_X = 0;
  static constexpr int ROLL_Y = 0;
  static constexpr int ROLL_W = 200;
  static constexpr int ROLL_H = 120;
  static constexpr int MAX_KEY = 512;
  //
  struct Key {
    bool isBlack;
    int whiteIndex;
    int blackLeftWhite;  // only for black keys
    int blackRightWhite;
  };
  uint8_t keys[MAX_KEY];
  struct MidiNote {
    uint8_t note;
    uint8_t velocity;
    uint32_t startTime;
    uint32_t endTime;
    bool active;
  };
  MidiNote midiHistory[MIDI_HISTORY_SIZE];
  uint16_t midiWriteIndex = 0;
  const uint32_t WINDOW_MS = 4000;
public:
  static CMG_PianoKey &getInstance() {
    static CMG_PianoKey instance;
    return instance;
  }

  void drawLogMidiNotesHistoric() {
    const char *notes[] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
    int nbChar = 4;
    int histPosY = 16;
    int nbNote = 15;
    for (int k = 0; k < nbNote; k++) {
      int idx = (midiWriteIndex - 1 - k + MIDI_HISTORY_SIZE) % MIDI_HISTORY_SIZE;
      MidiNote &n = midiHistory[idx];
      if (n.startTime == 0) {
        CMG_Screen::getInstance().drawString(" --- ", 8 * nbChar * k, histPosY, 1, CMG_BLACK);
        continue;
      }
      uint8_t noteIndex = n.note % 12;
      int octave = (n.note / 12) - 2;
      String txt = String(notes[noteIndex]) + String(octave);
      CMG_Screen::getInstance().drawString(CMG_Helper::getInstance().fill(txt, 5).c_str(), 8 * nbChar * k, histPosY, 1, CMG_Screen::CMG_WHITE);
      CMG_Screen::getInstance().drawString(CMG_Helper::getInstance().fill(txt, 5).c_str(), SCREEN_WIDTH / 2, 32 + 8 * k, 1, CMG_Screen::CMG_WHITE);
    }
  }

  void drawLogMidiNotes() {
    uint32_t now = nowMs();
    CMG_Screen::getInstance().fillSpriteRect(ROLL_X, ROLL_Y, ROLL_W, ROLL_H, CMG_Screen::CMG_GREY);
    for (int i = 0; i < MIDI_HISTORY_SIZE; i++) {
      MidiNote &n = midiHistory[i];
      if (n.startTime == 0) {
        continue;
      }
      uint32_t end = (n.active) ? now : n.endTime;
      // Skip old notes
      if (now - n.startTime > WINDOW_MS) {
        continue;
      }
      float tStart = (float)(now - n.startTime) / WINDOW_MS;
      float tEnd = (float)(now - end) / WINDOW_MS;
      int x1 = tStart * ROLL_W;
      int x2 = tEnd * ROLL_W;
      float noteStep = (float)ROLL_H / 128.0f;
      int y = ROLL_Y + ROLL_H - (int)(n.note * noteStep);
      int left = min(x1, x2);
      int right = max(x1, x2);
      int w = right - left;
      if (n.active && w < 2) {
        w = 2;
      }
      if (w <= 0) {
        continue;
      }
      if (w > ROLL_W) {
        continue;
      }
      int h = 2 + (n.velocity / 32);
      uint16_t color = (n.velocity > 60) ? CMG_Screen::CMG_WHITE_L3 : (n.velocity > 50) ? CMG_Screen::CMG_WHITE_L2
                                                                                        : CMG_Screen::CMG_WHITE_L1;
      CMG_Screen::getInstance().fillSpriteRect(left, y, w, h, color);
    }
    rollSprite.pushSprite(0, 64);
  }

  void drawLastNote() {
    String noteText = fill(String(noteReceived + " " + octaveReceived), 9);
    CMG_Screen::getInstance().drawString(noteText.c_str(), SCREEN_WIDTH / 2 - 8 * (noteText.length()) / 2, SCREEN_HEIGHT / 2 + 48 - 8, 2, CMG_Screen::CMG_WHITE);
    CMG_Screen::getInstance().drawString(CMG_Helper::getInstance().fill(String(velocityReceived), 3).c_str(), SCREEN_WIDTH - 3 * 16, SCREEN_HEIGHT / 2 + 48 - 8, 2, CMG_Screen::CMG_WHITE);
    // Velocity
    int maxH = 32;
    int barX = SCREEN_WIDTH - 32 - 3 * 16;
    int barY = SCREEN_HEIGHT / 2 + 48 - 8;
    int barW = 16;
    int level = constrain(map(velocityValueReceived, 0, 127, 0, maxH), 0, maxH);
    // Filled part (bottom)
    CMG_Screen::getInstance().fillRect(barX + 1, barY + (maxH - level), barW, level, CMG_Screen::CMG_WHITE);
    // black remaining part (top)
    CMG_Screen::getInstance().fillRect(barX + 1, barY, barW, maxH - level, CMG_BLACK);
    CMG_Screen::getInstance().fillRect(barX, barY, 1, maxH, CMG_Screen::CMG_WHITE);
  }

private:
  int findActiveNote(uint8_t note) {
    for (int i = 0; i < MIDI_HISTORY_SIZE; i++) {
      if (midiHistory[i].active && midiHistory[i].note == note && midiHistory[i].endTime == 0) {
        return i;
      }
    }
    return -1;
  }

  void pushNoteOn(uint8_t note, uint8_t velocity) {
    midiHistory[midiWriteIndex] = { note, velocity, nowMs(), 0, true };
    midiWriteIndex = (midiWriteIndex + 1) % MIDI_HISTORY_SIZE;
    midiLevels[note] = velocity;
    midiToTargets();
  }

  void pushNoteOff(uint8_t note) {
    int idx = findActiveNote(note);
    if (idx != -1) {
      midiHistory[idx].endTime = nowMs();
      midiHistory[idx].active = false;
      midiLevels[note] = 0;
      midiToTargets();
    }
  }
};