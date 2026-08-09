#include "BLEGateway.h"
#include "config.h"

BLEGateway::BLEGateway()
    : _connected(false), _transparent(false), _scanning(false),
      _targetFound(false), _targetAddrType(0), _scanStartTime(0), _notifyCallback(nullptr),
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

    _targetFound = false;

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&_scanCallbacks);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(50);
    pScan->start(30, false);

    _scanning = true;
    _scanStartTime = millis();
    Serial.println("[BLE] Scan started");
    Serial.print("[BLE] Looking for: ");
    Serial.println(TARGET_NAME);
}

void BLEGateway::stopScan() {
    if (!_scanning) return;
    NimBLEDevice::getScan()->stop();
    _scanning = false;
}

void BLEGateway::ScanCallbacks::onResult(NimBLEAdvertisedDevice* pDevice) {
    String name = pDevice->getName().c_str();
    Serial.print("[BLE] Found: ");
    Serial.println(name.length() > 0 ? name.c_str() : pDevice->getAddress().toString().c_str());

    if (name == TARGET_NAME) {
        BLEGateway* pGateway = BLEGateway::getInstance();

        if (pGateway->_targetFound) {
            return;
        }

        pGateway->_targetFound = true;
        pGateway->_targetAddress = NimBLEAddress(pDevice->getAddress());
        pGateway->_targetAddrType = pDevice->getAddressType();
        Serial.println("[BLE] Target found!");
        Serial.print("[BLE] Address: ");
        Serial.println(pGateway->_targetAddress.toString().c_str());
        Serial.print("[BLE] Address type: ");
        Serial.println(pGateway->_targetAddrType);

        pGateway->attemptConnection();
    }
}

void BLEGateway::attemptConnection() {
    if (!_targetFound) {
        Serial.println("[BLE] No target found");
        return;
    }

    stopScan();
    NimBLEDevice::getScan()->clearResults();

    if (NimBLEDevice::isBonded(_targetAddress)) {
        Serial.println("[BLE] Deleting existing bond...");
        NimBLEDevice::deleteBond(_targetAddress);
    }

    NimBLEClient* pClient = NimBLEDevice::getDisconnectedClient();
    if (!pClient) {
        Serial.println("[BLE] Creating new client...");
        pClient = NimBLEDevice::createClient();
    }
    if (!pClient) {
        Serial.println("[BLE] Failed to create client");
        delay(2000);
        startScan();
        return;
    }

    Serial.print("[BLE] Connecting to: ");
    Serial.println(_targetAddress.toString().c_str());

    pClient->setClientCallbacks(this);
    pClient->setConnectTimeout(10);

    pClient->connect(_targetAddress);

    uint32_t start = millis();
    while (!pClient->isConnected() && millis() - start < 5000) {
        delay(10);
    }

    if (!pClient->isConnected()) {
        int rc = pClient->getLastError();
        Serial.print("[BLE] Connection timeout, rc=");
        Serial.println(rc);
        pClient->disconnect();
        delay(2000);
        startScan();
        return;
    }

    Serial.println("[BLE] Connected!");
    _pClient = pClient;
    _connected = true;

    NimBLERemoteService* pService = pClient->getService(SERVICE_UUID);
    if (!pService) {
        Serial.println("[BLE] Service FFE0 not found");
        pClient->disconnect();
        delay(2000);
        startScan();
        return;
    }

    Serial.println("[BLE] Service found");

    _pWriteChar = pService->getCharacteristic(CHAR_WRITE_UUID);
    _pNotifyChar = pService->getCharacteristic(CHAR_NOTIFY_UUID);

    if (_pWriteChar) {
        Serial.println("[BLE] FFE1 found");
    } else {
        Serial.println("[BLE] FFE1 NOT found");
    }

    if (_pNotifyChar) {
        Serial.println("[BLE] FFE2 found, subscribing...");
        bool subResult = _pNotifyChar->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            if (_notifyCallback) {
                _notifyCallback(pData, length);
            }
        });
        Serial.print("[BLE] Subscribe result: ");
        Serial.println(subResult ? "success" : "failed");
    } else {
        Serial.println("[BLE] FFE2 NOT found");
    }

    if (_pWriteChar && _pNotifyChar) {
        _transparent = true;
        Serial.println("[BLE] === TRANSPARENT MODE ACTIVATED ===");
    } else {
        Serial.println("[BLE] Transparent mode failed");
        pClient->disconnect();
        delay(2000);
        startScan();
    }
}

void BLEGateway::onConnect(NimBLEClient* pClient) {
    Serial.println("[BLE] onConnect callback!");
    Serial.print("[BLE] Peer: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    _connected = true;
}

void BLEGateway::onDisconnect(NimBLEClient* pClient) {
    Serial.println("[BLE] Disconnected");
    _connected = false;
    _transparent = false;
    _pClient = nullptr;
    _pWriteChar = nullptr;
    _pNotifyChar = nullptr;
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
    bool result = _pWriteChar->writeValue(data, len, false);
    return result;
}

void BLEGateway::registerNotifyCallback(onNotifyCallback callback) {
    _notifyCallback = callback;
}
