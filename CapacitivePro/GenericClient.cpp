#include "BLEDevice.h"
#include "Arduino.h"
#include "GenericClient.h"

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
   /**
     * Called for each advertising BLE server.
     */
    public:
      GenericClient *myClient;
      
      void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());
    
        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID()) {
          DeviceType foundDevice = myClient->findDevice(advertisedDevice);
          
          if (foundDevice != NoDevice) { // Found interesting device
            if (myClient->foundDevices < MAX_CLIENTS) {
              int currentDevice = myClient->foundDevices;
              myClient->foundDevices++;
              myClient->advertisedDevices[currentDevice] = new BLEAdvertisedDevice(advertisedDevice);
              //myClient->doConnect = true;
              //myClient->doScan = true;
              myClient->deviceTypes[currentDevice] = foundDevice;
              myClient->foundServiceUUIDs[currentDevice] = new BLEUUID(advertisedDevice.getServiceUUID());
              //myClient->connectToServer(myClient->foundDevices-1);
            }
            else { // if found max number of devices, stop scan
              BLEDevice::getScan()->stop();
            }
          }
        }
      } // onResult
}; // MyAdvertisedDeviceCallbacks

class MyClientCallback : public BLEClientCallbacks {
  public:
    int deviceIndex;
    GenericClient *myClient;
    void onConnect(BLEClient* pclient) {
    }
  
    void onDisconnect(BLEClient* pclient) {
      //connected = false;
      myClient->connected[deviceIndex] = false;
      Serial.println("Disconnected!");
    }
};

DeviceType GenericClient::findDevice(BLEAdvertisedDevice device) {
  std::string uuid = device.getServiceUUID().toString();
  for (int i=1; i<DEVICE_TYPES_COUNT; i++) {
    if (uuid.find(serviceUUIDs[i]) != std::string::npos) { // service found
      bool alreadyConnected = false;
      for (int j=0; j<MAX_CLIENTS; j++) {
        if (deviceTypes[j] == i && connected[j])
          alreadyConnected = true;
      }
      if (!alreadyConnected)
        return (DeviceType)i;
    }
  }

  return NoDevice;
}

bool GenericClient::connectToServer(int deviceIndex) {
    BLEAdvertisedDevice* myDevice = advertisedDevices[deviceIndex];
    BLERemoteCharacteristic* pRemoteCharacteristic;
    
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    MyClientCallback *cb = new MyClientCallback();
    cb->deviceIndex = deviceIndex;
    cb->myClient = this; // important
    
    pClient->setClientCallbacks(cb);

    Serial.println("Connecting to server...");
    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLEUUID serviceUUID = *foundServiceUUIDs[deviceIndex];
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    std::map<std::string, BLERemoteCharacteristic*> *charsMap = pRemoteService->getCharacteristics();
    std::map<std::string, BLERemoteCharacteristic*>::iterator it;
    
    for ( it = charsMap->begin(); it != charsMap->end(); it++ )
    {
      std::string uuid = it->first;
      BLERemoteCharacteristic *characteristic = it->second;
      if (uuid.find(charUUIDs[deviceTypes[deviceIndex]]) != std::string::npos && characteristic->canWrite()) { // correct writeable characteristic found
        pRemoteCharacteristic = characteristic;
        break;
      }
    }
    
    //pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUIDs[deviceTypes[deviceIndex]].c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    /*if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }*/

    /*if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);*/

    if(pRemoteCharacteristic->canWrite()) {
      remoteChars[deviceIndex] = pRemoteCharacteristic;
      connected[deviceIndex] = true;
      Serial.println("Can write to characteristic");
      return true;
    }
    else {
      Serial.println("ERROR: Can't write to characteristic");
      return false;
    }
}

void GenericClient::updateLovense(float value, int deviceIndex) {
  //String newValue = "Time since boot: " + String(millis()/1000);
  
  int translatedValue = (int)(value*20.0+0.5);
  translatedValue = constrain(translatedValue, 0, 20);
  //if (translatedValue != lastIntegerValue[deviceIndex]) { // update only if needed
  String characteristicValue = "Vibrate:" + String(translatedValue) + ";";
  
  // Set the characteristic's value to be the array of bytes that is actually a string.
  remoteChars[deviceIndex]->writeValue((uint8_t*)characteristicValue.c_str(), characteristicValue.length(), false);
  //lastFloatValue[deviceIndex] = value;
}

void GenericClient::updateVibration(float value) {
  value = constrain(value, 0.0, 1.0);
  
  for (int i=0; i<foundDevices; i++) {
    /*if (fabs(lastFloatValue[i] - value) < 0.0001) { // filter useless messages
      lastFloatValue[i] = value;
      continue;
    }*/
    lastFloatValue[i] = value;
    
    if (connected[i]) {
      switch (deviceTypes[i]) {
        case Lovense:
          updateLovense(value, i);
          break;
        default:
          break;
      }
    }
  }
}

void GenericClient::initDevice(int deviceIndex) {
  switch (deviceTypes[deviceIndex]) {
    case Lovense:
      {
        String characteristicValue = "AutoSwith:Off:Off;"; // this typo is intentional
        remoteChars[deviceIndex]->writeValue((uint8_t*)characteristicValue.c_str(), characteristicValue.length(), true);
        String characteristicValue2 = "Light:on;";
        remoteChars[deviceIndex]->writeValue((uint8_t*)characteristicValue2.c_str(), characteristicValue2.length(), true);
        String characteristicValue3 = "ALight:Off;";
        remoteChars[deviceIndex]->writeValue((uint8_t*)characteristicValue3.c_str(), characteristicValue3.length(), true);
      }
      break;
    default:
      break;
  }
  
}

void GenericClient::startClient() {
  Serial.println("Starting BLEClient");
  //BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks *cb = new MyAdvertisedDeviceCallbacks();
  cb->myClient = this;
  pBLEScan->setAdvertisedDeviceCallbacks(cb);
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10, false); // 10 seconds scan
  for (int i=0; i<foundDevices; i++) {
    connectToServer(i);
  }
  for (int i=0; i<foundDevices; i++) {
    if (connected[i])
      initDevice(i);
  }

  updateVibration(0.5);
  delay(200);
  updateVibration(0.0);
  delay(500);
  updateVibration(0.5);
  delay(200);
  updateVibration(0.0);
  updateVibration(0.0);
  updateVibration(0.0);
}
