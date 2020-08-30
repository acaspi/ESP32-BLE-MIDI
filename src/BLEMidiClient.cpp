#include "BLEMidiClient.h"
#include <Arduino.h>            // to remove when serial prints will be removed

BLEMidiClient::BLEMidiClient(
    const std::string deviceName,
    void (*const onConnectCallback)(),
    void (*const onDisconnectCallback)()
    ) 
    :   BLEMidi(deviceName),
        onConnectCallback(onConnectCallback),
        onDisconnectCallback(onDisconnectCallback)
{}

void BLEMidiClient::begin()
{
    BLEMidi::begin();
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

int BLEMidiClient::scan()
{
    if(pBLEScan == nullptr)
        return 0;
    pBLEScan->clearResults();
    foundDevices = pBLEScan->start(3);
    Serial.printf("Found %d device(s)\n", foundDevices.getCount());
    for(int i=0; i<foundDevices.getCount(); i++)
        Serial.println(foundDevices.getDevice(i).toString().c_str());
    return foundDevices.getCount();
}

bool BLEMidiClient::connect(int deviceIndex)
{
    if(deviceIndex >= foundDevices.getCount())
        return false;
    Serial.printf("getDevice(%d)\n", deviceIndex);
    BLEAdvertisedDevice* device = new BLEAdvertisedDevice(foundDevices.getDevice(deviceIndex));
    Serial.printf("device = 0x%x\n", (void*)device);
    if(device == nullptr)
        return false;
    Serial.printf("Connecting to %s\n", device->getAddress().toString().c_str());
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(connected, onConnectCallback, onDisconnectCallback));
    Serial.println("pClient->connect()");
    if(!pClient->connect(device))
        return false;
    Serial.println("pClient->getService()");
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID.c_str());
    if(pRemoteService == nullptr)
        return false;
    Serial.println("pRemoteService->getCharacteristic()");
    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID.c_str());
    if(pRemoteCharacteristic == nullptr)
        return false;
    if(pRemoteCharacteristic->canNotify())
        CallbackRegister::registerCallback(pRemoteCharacteristic, [this](    // We have to use the CallbackRegister class to be able to call the receivePacket member function as a callback
            BLERemoteCharacteristic* pBLERemoteCharacteristic,          // A little bit overkill... ;)
            uint8_t* pData, 
            size_t length, 
            bool isNotify) {
                receivePacket(pData, length);
        });     
                                                                            
        pRemoteCharacteristic->registerForNotify(&CallbackRegister::mainCallback);
    connected=true;
    return true;
}

void BLEMidiClient::characteristicCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify) 
{

}

void BLEMidiClient::sendPacket(uint8_t *packet, uint8_t packetSize)
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
