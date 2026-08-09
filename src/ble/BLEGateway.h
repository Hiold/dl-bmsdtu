#ifndef BLE_GATEWAY_H
#define BLE_GATEWAY_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEClient.h>
#include <NimBLEAdvertisedDevice.h>

using onNotifyCallback = std::function<void(const uint8_t* data, size_t len)>;

class BLEGateway : public NimBLEClientCallbacks {
public:
    BLEGateway();

    bool init();
    bool connectToDevice();
    void disconnect();
    bool write(const uint8_t* data, size_t len);
    void registerNotifyCallback(onNotifyCallback callback);

    bool isConnected() const { return _connected; }
    bool isTransparent() const { return _transparent; }
    void setTransparent(bool transparent);

    void onConnect(NimBLEClient* pClient) override;
    void onDisconnect(NimBLEClient* pClient) override;
    void onServiceDiscoverComplete(NimBLERemoteService* pRemoteService) override;
    void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void handleServiceDiscover(NimBLERemoteService* pRemoteService);

    static BLEGateway* getInstance();

private:
    struct ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
        void onResult(NimBLEAdvertisedDevice* pDevice) override;
    };

    bool _connected = false;
    bool _transparent = false;
    onNotifyCallback _notifyCallback;
    ScanCallbacks _scanCallbacks;

    NimBLEAdvertisedDevice* _pDevice = nullptr;
    NimBLEClient* _pClient = nullptr;
    NimBLERemoteCharacteristic* _pWriteChar = nullptr;
    NimBLERemoteCharacteristic* _pNotifyChar = nullptr;
};

#endif
