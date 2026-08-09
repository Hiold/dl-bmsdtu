#include "BLEGateway.h"
#include "config.h"

BLEGateway::BLEGateway()
    : _connected(false), _transparent(false), _scanning(false),
      _scanStartTime(0), _notifyCallback(nullptr),
      _scanCallbacks(), _pClient(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

BLEGateway* BLEGateway::getInstance() {
    static BLEGateway instance;
    return &instance;
}

bool BLEGateway::init() {
    Serial.println("[BLE] Initializing...");
    NimBLEDevice::init("");
    Serial.println("[BLE] Done");
    return true;
}

void BLEGateway::startScan() {
    if (_scanning) {
        NimBLEDevice::getScan()->stop();
    }

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(nullptr);
    pScan->clearDuplicateCache();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(50);
    pScan->start(30, false);

    _scanning = true;
    _scanStartTime = millis();
    Serial.println("[BLE] Scan restarted");
    Serial.print("[BLE] Looking for: ");
    Serial.println(TARGET_MAC);
}

void BLEGateway::stopScan() {
    if (!_scanning) return;
    NimBLEDevice::getScan()->stop();
    _scanning = false;
}

void BLEGateway::connectToDevice() {
    NimBLEClient* pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(this);

    NimBLEAddress addr(TARGET_MAC);
    Serial.print("[BLE] Connecting to: ");
    Serial.println(addr.toString().c_str());

    if (pClient->connect(addr)) {
        Serial.println("[BLE] Connected!");
        _pClient = pClient;
        _connected = true;
        stopScan();

        std::vector<NimBLERemoteService*>* pServices = pClient->getServices();
        if (pServices) {
            Serial.print("[BLE] Services: ");
            Serial.println(pServices->size());
            for (NimBLERemoteService* pService : *pServices) {
                if (pService->getUUID().toString() == SERVICE_UUID) {
                    Serial.println("[BLE] Service FFE0 found!");
                    setupCharacteristics(pService);
                    break;
                }
            }
        }
    } else {
        Serial.println("[BLE] Connection failed!");
        pClient->disconnect();
        delay(1000);
        startScan();
    }
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    Serial.print("[BLE] Found: ");
    Serial.println(pDevice->getAddress().toString().c_str());

    if (pDevice->getAddress().toString() == TARGET_MAC) {
        Serial.println("[BLE] Target found!");
        BLEGateway* pGateway = BLEGateway::getInstance();
        pGateway->stopScan();

        NimBLEClient* pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(pGateway);

        NimBLEAddress addr(pDevice->getAddress());
        Serial.print("[BLE] Connecting: ");
        Serial.println(addr.toString().c_str());

        if (pClient->connect(addr)) {
            Serial.println("[BLE] Connected!");
            pGateway->_pClient = pClient;
            pGateway->_connected = true;

            std::vector<NimBLERemoteService*>* pServices = pClient->getServices();
            if (pServices) {
                for (NimBLERemoteService* pService : *pServices) {
                    if (pService->getUUID().toString() == SERVICE_UUID) {
                        Serial.println("[BLE] Service FFE0 found!");
                        pGateway->setupCharacteristics(pService);
                        break;
                    }
                }
            }
        } else {
            Serial.println("[BLE] Connection failed!");
            delay(1000);
            pGateway->startScan();
        }
    }
}

void BLEGateway::setupCharacteristics(NimBLERemoteService* pService) {
    if (!pService) {
        Serial.println("[BLE] Service null!");
        return;
    }

    _pWriteChar = pService->getCharacteristic(CHAR_WRITE_UUID);
    _pNotifyChar = pService->getCharacteristic(CHAR_NOTIFY_UUID);

    if (_pWriteChar) Serial.println("[BLE] FFE1 found");
    if (_pNotifyChar) {
        Serial.println("[BLE] FFE2 found, subscribing...");
        _pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            if (_notifyCallback) _notifyCallback(pData, length);
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
    startScan();
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
