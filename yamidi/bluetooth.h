/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */

#pragma once
//
#include "btscan.h"
//
class CMG_Bluetooth {
private:
  static constexpr uint16_t SERVICE_UUID = (uint16_t)0xFFF0;
  static constexpr uint16_t CHAR_UUID = (uint16_t)0xFFF1;
  static constexpr int MIDI_HISTORY_SIZE = 1024;
private:
  NimBLEAdvertisedDevice *targetDevice = nullptr;
  NimBLEClient *pClient = nullptr;
  NimBLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
  NimBLEUUID serviceUUID(SERVICE_UUID);
  NimBLEUUID charUUID(CHAR_UUID);
  volatile bool doConnect = false;
  NimBLEAdvertisedDevice *pendingDevice = nullptr;
  String dataReceived = "";
  String noteReceived = "...";
  String octaveReceived = "";
  uint8_t velocityValueReceived = 0;
  String velocityReceived = "..";
  String deviceName = "...";
private:
  static void notifyCallback(
    NimBLERemoteCharacteristic *pCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify) {
    for (size_t i = 0; i + 3 < length; i += 4) {
      uint32_t value =
        (uint32_t)pData[i] | (uint32_t)pData[i + 1] << 8 | (uint32_t)pData[i + 2] << 16 | (uint32_t)pData[i + 3] << 24;
      char buf[12];
      sprintf(buf, "%02X %02X %02X %02X",
              pData[i],
              pData[i + 1],
              pData[i + 2],
              pData[i + 3]);
      dataReceived = String(buf);
      uint8_t midinote = pData[i + 2];
      uint8_t action = pData[i];
      if (action == 0x09) {
        keys[midinote] = 1;
        pushNoteOn(midinote, pData[i + 3]);
      }
      if (action == 0x08 /*|| action == 0x0A*/) {
        keys[midinote] = 0;
        pushNoteOff(midinote);
      }
      if (action == 9) {
        // Note On event
        midiToNote(pData[i + 2]);
        velocityValueReceived = pData[i + 3];
        velocityReceived = String(pData[i + 3]);
      }
      if (action == 0x0B) {
        if (pData[i + 2] == 0x43) {
          p1 = pData[i + 3];
        }
        if (pData[i + 2] == 0x42) {
          p2 = pData[i + 3];
        }
        if (pData[i + 2] == 0x40) {
          p3 = pData[i + 3];
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
    NimBLEAdvertisedDevice *device = pendingDevice;
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
      bool ok = pClient->connect(device);
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
        bool ok = pRemoteCharacteristic->subscribe(true, CMG::Bluetooth::notifyCallback);
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
    pendingDevice = nullptr;
  }
};