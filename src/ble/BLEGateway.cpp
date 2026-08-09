#include "BLEGateway.h"
#include "config.h"

BLEGateway::BLEGateway()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _scanCallbacks(), _pClient(nullptr),
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

        BLEGateway* pGateway = BLEGateway::getInstance();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(pGateway);

        NimBLEAddress addr(pDevice->getAddress());
        if (pClient->connect(addr)) {
            pGateway->_pClient = pClient;
            pClient->getServiceByUUID(SERVICE_UUID, [pGateway](NimBLERemoteService* pService) {
                pGateway->setupServiceAndCharacteristics(pService);
            });
        }
    }
}

void BLEGateway::setupServiceAndCharacteristics(NimBLERemoteService* pService) {
    if (!pService) return;

    pService->getCharacteristicByUUID(CHAR_WRITE_UUID, [this](NimBLERemoteCharacteristic* pChar) {
        if (pChar) {
            this->_pWriteChar = pChar;
            this->_pNotifyChar = this->_pWriteChar;
        }
    });

    pService->getCharacteristicByUUID(CHAR_NOTIFY_UUID, [this](NimBLERemoteCharacteristic* pChar) {
        if (pChar) {
            this->_pNotifyChar = pChar;
            this->_pWriteChar = this->_pWriteChar ? this->_pWriteChar : pChar;
            this->_pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                if (this->_notifyCallback) {
                    this->_notifyCallback(pData, length);
                }
            });
            this->_transparent = true;
        }
    });
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    _connected = true;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    disconnect();
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

BLEGateway* BLEGateway::getInstance() {
    static BLEGateway instance;
    return &instance;
}
