#ifndef Server_h
#define Server_h
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

class SettingsServer {
  public:
    BLECharacteristic *cmdCharacteristic;
    void startServer();
};

#endif
