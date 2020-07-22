#ifndef GenericClient_h
#define GenericClient_h
#include <BLEDevice.h>

enum DeviceType { NoDevice, Lovense };
static const int DEVICE_TYPES_COUNT = 2;
static const int MAX_CLIENTS = 3;
// Lovense Domi 2 service
//static BLEUUID domiServiceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static const std::string lovenseServiceEnding = "4bd4-bbd5-a6920e4c5653";
// Lovense Domi 2 characteristic
static const std::string lovenseCharEnding = "4bd4-bbd5-a6920e4c5653";
static const std::string serviceUUIDs[2] = {"NULLDEVICE", lovenseServiceEnding};
static const std::string charUUIDs[2] = {"NULLDEVICE", lovenseCharEnding};

class GenericClient {
  public:
    //BLECharacteristic *cmdCharacteristic;
    int foundDevices = 0;
    int connectedDevices = 0;
    //boolean doConnect = false;
    boolean doScan = false;
    BLEAdvertisedDevice *advertisedDevices[MAX_CLIENTS];
    BLEUUID *foundServiceUUIDs[MAX_CLIENTS];
    BLERemoteCharacteristic *remoteChars[MAX_CLIENTS];
    DeviceType deviceTypes[MAX_CLIENTS];
    boolean connected[MAX_CLIENTS];
    void startClient();
    bool connectToServer(int deviceIndex);
    DeviceType findDevice(BLEAdvertisedDevice device);
    float lastFloatValue[MAX_CLIENTS];
    void updateLovense(float value, int deviceIndex);
    void updateVibration(float value);
    void initDevice(int deviceIndex);
};

#endif
