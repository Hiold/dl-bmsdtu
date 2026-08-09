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
    Serial.println("[BLE] Starting continuous scan...");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->setActiveScan(true);
    pScan->start(0, false);
    Serial.print("[BLE] Scanning for: ");
    Serial.println(TARGET_MAC);
    return true;
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    Serial.print("[BLE] Found: ");
    Serial.print(pDevice->getAddress().toString().c_str());
    if (strlen(pDevice->getName().c_str()) > 0) {
        Serial.print(" (");
        Serial.print(pDevice->getName().c_str());
        Serial.print(")");
    }
    Serial.println();

    if (pDevice->getAddress().toString() == TARGET_MAC) {
        Serial.println("[BLE] Target found! Stopping scan and connecting...");

        BLEGateway* pGateway = BLEGateway::getInstance();
        NimBLEDevice::getScan()->stop();

        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(pGateway);

        NimBLEAddress addr(pDevice->getAddress());
        if (pClient->connect(addr)) {
            Serial.println("[BLE] Connected! Discovering services...");
            pGateway->_pClient = pClient;
            pGateway->_connected = true;

            std::vector<NimBLERemoteService*>* pServices = pClient->getServices();
            if (pServices) {
                Serial.print("[BLE] Found ");
                Serial.print(pServices->size());
                Serial.println(" services");
                for (NimBLERemoteService* pService : *pServices) {
                    if (pService->getUUID().toString() == SERVICE_UUID) {
                        Serial.println("[BLE] Service FFE0 found!");
                        pGateway->setupCharacteristics(pService);
                        break;
                    }
                }
            } else {
                Serial.println("[BLE] No services found!");
            }
        } else {
            Serial.println("[BLE] Connection failed! Restarting scan...");
            NimBLEDevice::getScan()->start(0, false);
        }
    }
}

void BLEGateway::setupCharacteristics(NimBLERemoteService* pService) {
    if (!pService) {
        Serial.println("[BLE] Service is null!");
        return;
    }

    _pWriteChar = pService->getCharacteristic(CHAR_WRITE_UUID);
    _pNotifyChar = pService->getCharacteristic(CHAR_NOTIFY_UUID);

    if (_pWriteChar) {
        Serial.println("[BLE] FFE1 write characteristic found");
    } else {
        Serial.println("[BLE] FFE1 NOT found!");
    }

    if (_pNotifyChar) {
        Serial.println("[BLE] FFE2 notify characteristic found, subscribing...");
        _pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            if (_notifyCallback) {
                _notifyCallback(pData, length);
            }
        });
    } else {
        Serial.println("[BLE] FFE2 NOT found!");
    }

    if (_pWriteChar && _pNotifyChar) {
        _transparent = true;
        Serial.println("[BLE] === TRANSPARENT MODE ACTIVATED ===");
    } else {
        Serial.println("[BLE] Transparent mode FAILED");
    }
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    Serial.println("[BLE] onConnect");
    _connected = true;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    Serial.println("[BLE] === DISCONNECTED ===");
    disconnect();
    Serial.println("[BLE] Restarting scan...");
    NimBLEDevice::getScan()->start(0, false);
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
