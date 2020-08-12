#ifndef GenericClient_h
#define GenericClient_h
#include <BLEDevice.h>

enum DeviceType { NoDevice, Lovense, Monsterpub };
static const int DEVICE_TYPES_COUNT = 3;

class FoundDevice {
  public:
    DeviceType type;
    BLEUUID *uuid;
  
    FoundDevice(DeviceType type_, BLEUUID *uuid_) {
      type = type_;
      uuid = uuid_;
    }
};

static const int MAX_CLIENTS = 3;

// Lovense Domi 2 service
//static BLEUUID domiServiceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static const std::string lovenseServiceEnding = "4bd4-bbd5-a6920e4c5653";
// Lovense Domi 2 characteristic
static const std::string lovenseCharEnding = "4bd4-bbd5-a6920e4c5653";

static const std::string monsterpubServiceEnding = "6000-0000-1000-8000-00805f9b34fb";
static const std::string monsterpubCharEnding = "6001-0000-1000-8000-00805f9b34fb";

static const std::string serviceUUIDs[3] = {"NULLDEVICE", lovenseServiceEnding, monsterpubServiceEnding};
static const std::string charUUIDs[3] = {"NULLDEVICE", lovenseCharEnding, monsterpubCharEnding};

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
    FoundDevice * findDevice(BLEAdvertisedDevice device);
    //bool hasEnding (std::string const &fullString, std::string const &ending);
    float lastFloatValue[MAX_CLIENTS];
    void updateLovense(float value, int deviceIndex);
    void updateMonsterpub(float value, int deviceIndex);
    void updateVibration(float value, float pos=0.0);
    void initDevice(int deviceIndex);
};

#endif
