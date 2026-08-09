#include "BLEGateway.h"
#include "config.h"

BLEGateway::BLEGateway()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _scanCallbacks(), _pDevice(nullptr), _pClient(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

bool BLEGateway::init() {
    NimBLEDevice::init("");
    return true;
}

bool BLEGateway::connectToDevice() {
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->start(5, false);
    return true;
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    if (pDevice->getAddress().toString() == TARGET_MAC) {
        NimBLEDevice::getScan()->stop();

        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(BLEGateway::getInstance());

        NimBLEAddress addr(pDevice->getAddress());
        if (pClient->connect(addr)) {
            pClient->discoverServices();
        }
    }
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    _connected = true;
    _pClient = pClient;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    disconnect();
}

void BLEGateway::onServiceDiscoverComplete(NimBLERemoteService* pRemoteService) {
    handleServiceDiscover(pRemoteService);
}

void BLEGateway::notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (_notifyCallback) {
        _notifyCallback(pData, length);
    }
}

void BLEGateway::disconnect() {
    _connected = false;
    _transparent = false;
    if (_pClient) {
        _pClient->disconnect();
    }
    _pClient = nullptr;
    _pWriteChar = nullptr;
    _pNotifyChar = nullptr;
}

bool BLEGateway::write(const uint8_t* data, size_t len) {
    if (!_transparent || !_pWriteChar) return false;
    _pWriteChar->writeValue(data, len, false);
    return true;
}

void BLEGateway::registerNotifyCallback(onNotifyCallback callback) {
    _notifyCallback = callback;
}

void BLEGateway::setTransparent(bool transparent) {
    _transparent = transparent;
}

BLEGateway* BLEGateway::getInstance() {
    static BLEGateway instance;
    return &instance;
}

void BLEGateway::handleServiceDiscover(NimBLERemoteService* pRemoteService) {
    if (pRemoteService && pRemoteService->getUUID().toString() == SERVICE_UUID) {
        NimBLERemoteCharacteristic* pWriteChar = pRemoteService->getCharacteristic(CHAR_WRITE_UUID);
        NimBLERemoteCharacteristic* pNotifyChar = pRemoteService->getCharacteristic(CHAR_NOTIFY_UUID);
        
        if (pWriteChar && pNotifyChar) {
            _pWriteChar = pWriteChar;
            _pNotifyChar = pNotifyChar;
            _pNotifyChar->registerForNotify([this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->notifyCallback(pChar, pData, length, isNotify);
            });
            _transparent = true;
        }
    }
}
