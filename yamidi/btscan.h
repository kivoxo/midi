/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
#pragma once
//
class ScanCallbacks : public NimBLEScanCallbacks {
private:
   static constexpr int BT_SERVER_NAME = "ESP32_MIDI";
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
      pendingDevice = const_cast<NimBLEAdvertisedDevice *>(advertisedDevice);
      doConnect = true;
      NimBLEDevice::getScan()->stop();
      deviceName = advertisedDevice->getName().c_str();
    }
  }
};