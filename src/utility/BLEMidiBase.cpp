#include <Arduino.h>
#include "BLEMidiBase.h"

void BLEMidi::begin(const std::string deviceName,  const std::string devicePeerName)
{
    this->deviceName = deviceName;
    BLEDevice::init(deviceName);
}

bool BLEMidi::isConnected()
{
    return connected;
}