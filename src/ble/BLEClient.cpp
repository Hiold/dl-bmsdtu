#include "BLEClient.h"
#include "config.h"

BLEClient::BLEClient()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _scanCallbacks(), _pDevice(nullptr), _pClient(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

bool BLEClient::init() {
    BLEDevice::init("");
    return true;
}

bool BLEClient::connectToDevice() {
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->start(5, false);
    return true;
}

void BLEClient::ScanCallbacks::onResult(BLEAdvertisedDevice* pDevice) {
    if (pDevice->getAddress().toString() == TARGET_MAC) {
        BLEDevice::getScan()->stop();
        BLEDevice::getScan()->clearAdvertisedDeviceCallbacks();

        BLEClient* pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(BLEClient::getInstance());

        BLEAddress addr(pDevice->getAddress());
        if (pClient->connect(addr)) {
            BLEDevice::setConnectedClient(pClient);
            pClient->discoverServices();
        }
    }
}

void BLEClient::onConnect(BLEClient* pClient) {
    _connected = true;
    _pClient = pClient;
}

void BLEClient::onDisconnect(BLEClient* pClient) {
    disconnect();
}

void BLEClient::onServiceDiscoverComplete(BLERemoteService* pRemoteService) {
    handleServiceDiscover(pRemoteService);
}

void BLEClient::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (_notifyCallback) {
        _notifyCallback(pData, length);
    }
}

void BLEClient::disconnect() {
    _connected = false;
    _transparent = false;
    if (_pClient) {
        _pClient->disconnect();
    }
    _pClient = nullptr;
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

void BLEClient::setTransparent(bool transparent) {
    _transparent = transparent;
}

BLEClient* BLEClient::getInstance() {
    static BLEClient instance;
    return &instance;
}

void BLEClient::handleServiceDiscover(BLERemoteService* pRemoteService) {
    if (pRemoteService && pRemoteService->getUUID().toString() == SERVICE_UUID) {
        BLERemoteCharacteristic* pWriteChar = pRemoteService->getCharacteristic(CHAR_WRITE_UUID);
        BLERemoteCharacteristic* pNotifyChar = pRemoteService->getCharacteristic(CHAR_NOTIFY_UUID);
        
        if (pWriteChar && pNotifyChar) {
            _pWriteChar = pWriteChar;
            _pNotifyChar = pNotifyChar;
            _pNotifyChar->registerForNotify([this](BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                this->notifyCallback(pChar, pData, length, isNotify);
            });
            _transparent = true;
        }
    }
}