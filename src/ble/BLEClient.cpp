#include "BLEClient.h"
#include "config.h"

BLEClient::BLEClient()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

bool BLEClient::init() {
    BLEDevice::init("");
    BLEDevice::setCustomGapEventHandler([](const BLEGapEvent& event) {
        // Handle gap events if needed
    });
    return true;
}

bool BLEClient::connectToDevice() {
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new class : public BLEAdvertisedDeviceCallbacks {
        void onResult(BLEAdvertisedDevice* pDevice) override {
            if (pDevice->getAddress().toString() == TARGET_MAC) {
                BLEDevice::getScan()->stop();
                // Store device and trigger connection
            }
        }
    });
    pScan->start(5);
    return true;
}

void BLEClient::disconnect() {
    _connected = false;
    _transparent = false;
    _pWriteChar = nullptr;
    _pNotifyChar = nullptr;
}

bool BLEClient::write(const uint8_t* data, size_t len) {
    if (!_transparent || !_pWriteChar) return false;
    _pWriteChar->writeValue(data, len, false);
    return true;
}

void BLEClient::registerNotifyCallback(onNotifyCallback callback) {
    _notifyCallback = callback;
}

void BLEClient::onConnect(BLEClient* pClient) {
    _connected = true;
}

void BLEClient::onDisconnect(BLEClient* pClient) {
    disconnect();
}

void BLEClient::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (_notifyCallback) {
        _notifyCallback(pData, length);
    }
}