#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Arduino.h"
#include "server.h"
#include "constants.h"
#include <BLESecurity.h>

void setStaticPIN(BLESecurity *sec, uint32_t pin){
    uint32_t passkey = pin;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    sec->setCapability(ESP_IO_CAP_OUT);
    sec->setKeySize();
    //sec->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
    sec->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND); // secure connection with MITM protection, bonding enabled
    sec->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void SettingsServer::startServer() {
  //Serial.begin(115200);
  Serial.println("Starting BLE work!");
  
  //BLEDevice::setPower(ESP_PWR_LVL_N12);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHAR_COMMANDSTRING,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  
  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM); // only during encrypted connection with MITM protection
  pCharacteristic->setValue(HELP_STRING);
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);  // min about 5 seconds
  pAdvertising->setMaxInterval(0x1500);  // max about 8 seconds
  //pAdvertising->setMinPreferred(0x06);  // min about 5 seconds
  pAdvertising->setMaxPreferred(0x1500);  // max about 8 seconds
  pAdvertising->setAppearance(64);
  BLEDevice::startAdvertising();
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N12);
  Serial.println("cmdCharacteristic defined");
  this->cmdCharacteristic = pCharacteristic;

  BLESecurity *pSecurity = new BLESecurity(); // enable bonding with pin
  setStaticPIN(pSecurity, 277227); 
  
}
