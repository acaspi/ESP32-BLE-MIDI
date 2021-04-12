#include "BLEMidiClient.h"

static bool isDataReceived = false;
static String strData;

void BLEMidiClientClass::begin(const std::string deviceName, const std::string deviceToLook)
{
    BLEMidi::begin(deviceName);
    deviceToLookFor = deviceToLook;
}

int BLEMidiClientClass::scan()
{
    debug.println("Beginning scan...");
    pBLEScan = BLEDevice::getScan();
    if(pBLEScan == nullptr)
        return 0;
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->clearResults();
    foundMidiDevices.clear();
    BLEScanResults foundDevices = pBLEScan->start(3);
    debug.printf("Found %d BLE device(s)\n", foundDevices.getCount());
    for(int i=0; i<foundDevices.getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices.getDevice(i);
        auto deviceStr = "name = \"" + device.getName() + "\", address = "  + device.getAddress().toString();
        if (device.haveServiceUUID() && device.isAdvertisingService(BLEUUID(MIDI_SERVICE_UUID))) {
            if (device.getName().find(deviceToLookFor) != std::string::npos)
            {
                debug.println((" - Found BLE MIDI device looking for: " + deviceStr).c_str());
                foundMidiDevices.push_back(device);
            }
        }
        else
            debug.printf((" - Other type of BLE device : " + deviceStr).c_str());
        debug.printf("Total of BLE MIDI devices : %d\n", foundMidiDevices.size());
    }
    return foundMidiDevices.size();
}

BLEAdvertisedDevice* BLEMidiClientClass::getScannedDevice(uint32_t deviceIndex)
{
    if(deviceIndex >= foundMidiDevices.size()) {
        debug.println("Scanned device not found because requested index is greater than the devices list");
        return nullptr;
    }
    return &foundMidiDevices.at(deviceIndex);
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
    strData.reserve(length);
    for(int i=0; i<length; i++){
      strData +=(char)pData[i];
    }
    
    /* if (length < BLE_MAX_PACKET_LEN)
    {
        memcpy(incomingData, pData, length);
        dataLength = length;
       
    } */

    isDataReceived = true;
}

void BLEMidiClientClass::readRcvPackets()
{
    if (isDataReceived)
    {
        BLEMidiClient.receivePacket((uint8_t *)(strData.c_str()), strData.length());
        strData.clear();

        isDataReceived = false;
    }
}

bool BLEMidiClientClass::connect(uint32_t deviceIndex)
{
    debug.printf("Connecting to device number %d\n", deviceIndex);
    if(deviceIndex >= foundMidiDevices.size()) {
        debug.println("Cannot connect : device index is greater than the size of the MIDI devices lists.");
        return false;
    }
    BLEAdvertisedDevice* device = new BLEAdvertisedDevice(foundMidiDevices.at(deviceIndex));
    if(device == nullptr)
        return false;
    debug.printf("Address of the device : %s\n", device->getAddress().toString().c_str());
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(connected, onConnectCallback, onDisconnectCallback));
    debug.println("pClient->connect()");
    if(!pClient->connect(device))
        return false;
    debug.println("pClient->getService()");
    BLERemoteService* pRemoteService = pClient->getService(MIDI_SERVICE_UUID.c_str());
    if(pRemoteService == nullptr) {
        debug.println("Couldn't find remote service");
        return false;
    }
    debug.println("pRemoteService->getCharacteristic()");
    pRemoteCharacteristic = pRemoteService->getCharacteristic(MIDI_CHARACTERISTIC_UUID.c_str());
    if(pRemoteCharacteristic == nullptr) {
        debug.println("Couldn't find remote characteristic");
        return false;
    }
    debug.println("Registering characteristic callback");
    if(pRemoteCharacteristic->canNotify()) {                                                                        
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
    connected=true;
    return true;
}

void BLEMidiClientClass::setOnConnectCallback(void (*const onConnectCallback)())
{
    this->onConnectCallback = onConnectCallback;
}
void BLEMidiClientClass::setOnDisconnectCallback(void (*const onDisconnectCallback)())
{
    this->onDisconnectCallback = onDisconnectCallback;
}

void BLEMidiClientClass::sendPacket(uint8_t *packet, uint8_t packetSize)
{
    if(!connected)
        return;
    pRemoteCharacteristic->writeValue(packet, packetSize);
}

ClientCallbacks::ClientCallbacks(
    bool& connected,
    void (*onConnectCallback)(), 
    void (*onDisconnectCallback)()
) :     connected(connected),
        onConnectCallback(onConnectCallback),
        onDisconnectCallback(onDisconnectCallback)
{}

void ClientCallbacks::onConnect(BLEClient *pClient)
{
    connected = true;
    if(onConnectCallback != nullptr)
        onConnectCallback();
}

void ClientCallbacks::onDisconnect(BLEClient *pClient)
{
    connected = false;
    if(onDisconnectCallback != nullptr)
        onDisconnectCallback();
}

BLEMidiClientClass BLEMidiClient;