#include "BLEGateway.h"
#include "config.h"

BLEGateway::BLEGateway()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _scanCallbacks(), _pClient(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

BLEGateway* BLEGateway::getInstance() {
    static BLEGateway instance;
    return &instance;
}

bool BLEGateway::init() {
    Serial.println("[BLE] Initializing...");
    NimBLEDevice::init("");
    Serial.println("[BLE] Device initialized");
    return true;
}

bool BLEGateway::connectToDevice() {
    Serial.println("[BLE] Starting scan for target device...");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->start(5, false);
    Serial.println("[BLE] Scan started, searching for: " TARGET_MAC);
    return true;
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    Serial.print("[BLE] Found device: ");
    Serial.println(pDevice->getAddress().toString().c_str());

    if (pDevice->getAddress().toString() == TARGET_MAC) {
        Serial.println("[BLE] Target device found! Stopping scan...");
        NimBLEDevice::getScan()->stop();

        BLEGateway* pGateway = BLEGateway::getInstance();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(pGateway);

        NimBLEAddress addr(pDevice->getAddress());
        Serial.print("[BLE] Connecting to: ");
        Serial.println(addr.toString().c_str());

        if (pClient->connect(addr)) {
            Serial.println("[BLE] Connected! Getting services...");
            pGateway->_pClient = pClient;
            pGateway->_connected = true;

            std::vector<NimBLERemoteService*>* pServices = pClient->getServices();
            if (pServices) {
                Serial.print("[BLE] Found ");
                Serial.print(pServices->size());
                Serial.println(" services");
                for (NimBLERemoteService* pService : *pServices) {
                    Serial.print("[BLE] Service UUID: ");
                    Serial.println(pService->getUUID().toString().c_str());
                    if (pService->getUUID().toString() == SERVICE_UUID) {
                        Serial.println("[BLE] Target service FFE0 found!");
                        pGateway->setupCharacteristics(pService);
                        break;
                    }
                }
            } else {
                Serial.println("[BLE] No services found!");
            }
        } else {
            Serial.println("[BLE] Connection failed!");
        }
    }
}

void BLEGateway::setupCharacteristics(NimBLERemoteService* pService) {
    if (!pService) {
        Serial.println("[BLE] Service is null!");
        return;
    }

    Serial.println("[BLE] Getting characteristics...");

    _pWriteChar = pService->getCharacteristic(CHAR_WRITE_UUID);
    _pNotifyChar = pService->getCharacteristic(CHAR_NOTIFY_UUID);

    if (_pWriteChar) {
        Serial.println("[BLE] Write characteristic FFE1 found");
    } else {
        Serial.println("[BLE] Write characteristic FFE1 NOT found!");
    }

    if (_pNotifyChar) {
        Serial.println("[BLE] Notify characteristic FFE2 found, subscribing...");
        _pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            if (_notifyCallback) {
                _notifyCallback(pData, length);
            }
            Serial.print("[BLE] Received notify, len=");
            Serial.println(length);
        });
    } else {
        Serial.println("[BLE] Notify characteristic FFE2 NOT found!");
    }

    if (_pWriteChar && _pNotifyChar) {
        _transparent = true;
        Serial.println("[BLE] === TRANSPARENT MODE ACTIVATED ===");
    } else {
        Serial.println("[BLE] Transparent mode FAILED - missing characteristics");
    }
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    Serial.println("[BLE] onConnect callback");
    _connected = true;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    Serial.println("[BLE] === DISCONNECTED ===");
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
    Serial.println("[BLE] Disconnected and cleaned up");
}

bool BLEGateway::write(const uint8_t* data, size_t len) {
    if (!_transparent || !_pWriteChar) return false;
    _pWriteChar->writeValue(data, len, false);
    Serial.print("[BLE] Wrote to BLE, len=");
    Serial.println(len);
    return true;
}

void BLEGateway::registerNotifyCallback(onNotifyCallback callback) {
    _notifyCallback = callback;
}
