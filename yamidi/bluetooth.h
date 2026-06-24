/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */

#pragma once
//
#include "spectre.h"
#include "pianokey.h"
//
class CMG_Bluetooth {
private:
#define BT_SERVER_NAME "ESP32_MIDI"
  inline static NimBLEUUID serviceUUID = NimBLEUUID((uint16_t)0xFFF0);
  inline static NimBLEUUID charUUID = NimBLEUUID((uint16_t)0xFFF1);
  inline static volatile bool doConnect = false;
  inline static NimBLEAdvertisedDevice *pendingDevice = nullptr;
  inline static String deviceName = "...";
  NimBLEAdvertisedDevice *targetDevice = nullptr;
  NimBLEClient *pClient = nullptr;
  NimBLERemoteCharacteristic *pRemoteCharacteristic = nullptr;

private:
  class ScanCallbacks : public NimBLEScanCallbacks {
  private:
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
      Serial.println("---------------");
      Serial.print("MAC: ");
      Serial.println(advertisedDevice->getAddress().toString().c_str());
      if (advertisedDevice->haveName()) {
        Serial.print("Name: ");
        Serial.println(advertisedDevice->getName().c_str());
      }
      if (advertisedDevice->haveServiceUUID()) {
        Serial.print("Service UUID: ");
        Serial.println(advertisedDevice->getServiceUUID().toString().c_str());
      }
      if ((advertisedDevice->haveName() && advertisedDevice->getName() == BT_SERVER_NAME) || advertisedDevice->isAdvertisingService(serviceUUID)) {
        Serial.println("Target device found!");
        //pendingDevice = const_cast<NimBLEAdvertisedDevice *>(advertisedDevice);
        pendingDevice = new NimBLEAdvertisedDevice(*advertisedDevice);
        doConnect = true;
        NimBLEDevice::getScan()->stop();
        deviceName = advertisedDevice->getName().c_str();
      }
    }
  };

  static void pushNoteOn(uint8_t note, uint8_t velocity) {
    CMG_Piano::getInstance().keyStatus(note, 1);
    CMG_PianoKey::getInstance().midiToNote(note, velocity);
    CMG_PianoKey::getInstance().onMidiHistory(note, velocity);
    CMG_Spectre::getInstance().midiToTargets(note, velocity);
  }

  static void pushNoteOff(uint8_t note) {
    CMG_Piano::getInstance().keyStatus(note, 0);
    int idx = CMG_PianoKey::getInstance().offMidiHistory(note);
    if (idx != -1) {
      CMG_Spectre::getInstance().midiToTargets(note, 0);
    }
  }

  static void notifyCallback(NimBLERemoteCharacteristic *pCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
    for (size_t i = 0; i + 3 < length; i += 4) {
      uint8_t cin = pData[i];
      uint8_t status = pData[i + 1];
      uint8_t note = pData[i + 2];
      uint8_t velocity = pData[i + 3];
      char buf[12];
      sprintf(buf, "%02X %02X %02X %02X", cin, status, note, velocity);
      String dataReceived = String(buf);
      if (cin == 0x09) {
        pushNoteOn(note, velocity);
      } else if (cin == 0x08 /*|| cin == 0x0A*/) {
        pushNoteOff(note);
      } else if (cin == 0x0B) {
        if (note == 0x43) {
          CMG_Substain::getInstance().setSubstain(0, velocity);
        } else if (note == 0x42) {
          CMG_Substain::getInstance().setSubstain(1, velocity);
        } else if (note == 0x40) {
          CMG_Substain::getInstance().setSubstain(2, velocity);
        }
      }
    }
  }

public:
  static CMG_Bluetooth &getInstance() {
    static CMG_Bluetooth instance;
    return instance;
  }

  void initBLEClient() {
    NimBLEDevice::init("ESP32_MIDI");
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new ScanCallbacks());  // FIXED
    pScan->setActiveScan(true);
    pScan->start(0, false);  // continuous scan
    Serial.println("BLE scanning started...");
  }

  bool connectToServer() {
    if (doConnect && pendingDevice) {
      doConnect = false;
      Serial.println("Connecting to BLE device...");
      delay(200);  // CRITICAL for BLE stack stability
      if (pClient) {
        if (pClient->isConnected()) {
          Serial.println("Already connected");
          pendingDevice = nullptr;
          return true;
        }
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
      }
      pClient = NimBLEDevice::createClient();
      bool ok = pClient->connect(pendingDevice);
      if (!ok) {
        Serial.println("CONNECT FAILED");
        pendingDevice = nullptr;
        return false;
      }
      Serial.println("CONNECTED SUCCESSFULLY");
      pClient->discoverAttributes();  // IMPORTANT
      delay(200);
      auto services = pClient->getServices();
      Serial.println("=== SERVICES FOUND ===");
      for (auto &s : services) {
        Serial.println(s->getUUID().toString().c_str());
      }
      NimBLERemoteService *pService = pClient->getService(serviceUUID);
      if (pService == nullptr) {
        Serial.println("Service not found");
        pendingDevice = nullptr;
        return false;
      } else {
        Serial.printf("Service found: %s\n", serviceUUID.toString().c_str());
      }
      pRemoteCharacteristic = pService->getCharacteristic(charUUID);
      if (pRemoteCharacteristic == nullptr) {
        Serial.println("Characteristic not found");
        pendingDevice = nullptr;
        return false;
      } else {
        Serial.printf("Characteristic found: %s\n", charUUID.toString().c_str());
      }
      if (pRemoteCharacteristic->canNotify() || pRemoteCharacteristic->canIndicate()) {
        bool ok = pRemoteCharacteristic->subscribe(true, CMG_Bluetooth::notifyCallback);
        if (!ok) {
          Serial.println("Subscribe FAILED");
          pendingDevice = nullptr;
          return false;
        } else {
          Serial.println("Subscribed OK");
        }
      }
      Serial.println("BLE connected and subscribed!");
      pendingDevice = nullptr;
      return true;
   }
   return true;
  }
};