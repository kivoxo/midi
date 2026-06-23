/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#include "FS.h"
#include <AsyncTCP.h>
#include <NimBLEDevice.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include "CMG_Screen.h"
#include "spectre.h"
#include "helper.h"
#include "piano.h"
#include "substain.h"
#include "pianokey.h"
#include "bluetooth.h"
//
static constexpr String TITLE = "MIDI VIEWER";

void setup() {
  CMG_Helper::getInstance().initSerial();
  CMG_Screen::getInstance().initScreen();
  CMG_Bluetooth::getInstance().initBLEClient();
  CMG_Piano::getInstance().buildKeyboard();
}

void loop() {
  CMG_Bluetooth::getInstance().connectToServer();
  //
  CMG_Spectre::getInstance().updateBands();
  //draw
  CMG_Screen::getInstance().drawTitle(TITLE);
  CMG_Piano::getInstance().drawPiano();
  CMG_PianoKey::getInstance().drawLastNote();
  CMG_Substain::getInstance().drawSubstain();
  CMG_PianoKey::getInstance().drawLogMidiNotesHistoric();
  CMG_PianoKey::getInstance().drawLogMidiNotes();
  CMG_Spectre::getInstance().drawSpectrum();
}