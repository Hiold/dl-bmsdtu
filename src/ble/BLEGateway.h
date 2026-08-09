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
    void onConnectFail(NimBLEClient* pClient, int reason) override;
    void onDisconnect(NimBLEClient* pClient, int reason) override;
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) override;

private:
    struct ScanCallbacks : public NimBLEScanCallbacks {
        void onResult(const NimBLEAdvertisedDevice* pDevice) override;
        void onDiscovered(const NimBLEAdvertisedDevice* pDevice) override;
        void onScanEnd(const NimBLEScanResults& results, int reason) override;
    };

    void attemptConnection();

    bool _connected = false;
    bool _transparent = false;
    bool _scanning = false;
    bool _targetFound = false;
    uint8_t _targetAddrType = 0;
    uint32_t _scanStartTime = 0;
    onNotifyCallback _notifyCallback;
    ScanCallbacks _scanCallbacks;

    NimBLEAddress _targetAddress;
    NimBLEClient* _pClient = nullptr;
    NimBLERemoteCharacteristic* _pWriteChar = nullptr;
    NimBLERemoteCharacteristic* _pNotifyChar = nullptr;
};

#endif
