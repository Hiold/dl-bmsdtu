#ifndef BLE_GATEWAY_H
#define BLE_GATEWAY_H

#include <Arduino.h>
#include <vector>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEClient.h>
#include <NimBLEAdvertisedDevice.h>

using onNotifyCallback = std::function<void(const uint8_t* data, size_t len)>;

class BLEGateway : public NimBLEClientCallbacks {
public:
    BLEGateway();

    static BLEGateway* getInstance();

    bool init();
    void startScan();
    void stopScan();
    void disconnect();
    bool write(const uint8_t* data, size_t len);
    void registerNotifyCallback(onNotifyCallback callback);

    bool isConnected() const { return _connected; }
    bool isTransparent() const { return _transparent; }

    void onConnect(NimBLEClient* pClient) override;
    void onDisconnect(NimBLEClient* pClient) override;

private:
    struct ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
        void onResult(NimBLEAdvertisedDevice* pDevice) override;
    };

    void attemptConnection();

    bool _connected = false;
    bool _transparent = false;
    bool _scanning = false;
    bool _targetFound = false;
    uint32_t _scanStartTime = 0;
    onNotifyCallback _notifyCallback;
    ScanCallbacks _scanCallbacks;

    NimBLEAddress _targetAddress;
    NimBLEClient* _pClient = nullptr;
    NimBLERemoteCharacteristic* _pWriteChar = nullptr;
    NimBLERemoteCharacteristic* _pNotifyChar = nullptr;
};

#endif
