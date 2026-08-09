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
    NimBLEDevice::deinitScan();
    delay(100);

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(50);
    pScan->start(30, false);

    Serial.println("[BLE] Scan started for 30s...");
    Serial.print("[BLE] Searching for: ");
    Serial.println(TARGET_MAC);
    return true;
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    Serial.print("[BLE] Found: ");
    Serial.println(pDevice->getAddress().toString().c_str());

    if (pDevice->getAddress().toString() == TARGET_MAC) {
        Serial.println("[BLE] Target found!");
        NimBLEDevice::getScan()->stop();

        BLEGateway* pGateway = BLEGateway::getInstance();
        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(pGateway);

        NimBLEAddress addr(pDevice->getAddress());
        Serial.print("[BLE] Connecting to: ");
        Serial.println(addr.toString().c_str());

        if (pClient->connect(addr)) {
            Serial.println("[BLE] Connected!");
            pGateway->_pClient = pClient;
            pGateway->_connected = true;

            std::vector<NimBLERemoteService*>* pServices = pClient->getServices();
            if (pServices) {
                Serial.print("[BLE] Services: ");
                Serial.println(pServices->size());
                for (NimBLERemoteService* pService : *pServices) {
                    if (pService->getUUID().toString() == SERVICE_UUID) {
                        Serial.println("[BLE] Service FFE0 found!");
                        pGateway->setupCharacteristics(pService);
                        break;
                    }
                }
            } else {
                Serial.println("[BLE] No services!");
            }
        } else {
            Serial.println("[BLE] Connection failed!");
            pClient->disconnect();
            delay(1000);
            BLEGateway::getInstance()->connectToDevice();
        }
    }
}

void BLEGateway::ScanCallbacks::onScanEnd(NimBLEScanResults& results) {
    Serial.println("[BLE] Scan ended");
    BLEGateway* pGateway = BLEGateway::getInstance();
    if (!pGateway->_transparent && !pGateway->_connected) {
        Serial.println("[BLE] No target found, restarting scan...");
        delay(1000);
        pGateway->connectToDevice();
    }
}

void BLEGateway::setupCharacteristics(NimBLERemoteService* pService) {
    if (!pService) {
        Serial.println("[BLE] Service null!");
        return;
    }

    _pWriteChar = pService->getCharacteristic(CHAR_WRITE_UUID);
    _pNotifyChar = pService->getCharacteristic(CHAR_NOTIFY_UUID);

    if (_pWriteChar) {
        Serial.println("[BLE] FFE1 found");
    }
    if (_pNotifyChar) {
        Serial.println("[BLE] FFE2 found, subscribing...");
        _pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            if (_notifyCallback) {
                _notifyCallback(pData, length);
            }
        });
    }

    if (_pWriteChar && _pNotifyChar) {
        _transparent = true;
        Serial.println("[BLE] === TRANSPARENT MODE ===");
    }
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    _connected = true;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    Serial.println("[BLE] Disconnected");
    disconnect();
    delay(1000);
    connectToDevice();
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
