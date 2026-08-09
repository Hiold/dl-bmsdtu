#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLERemoteCharacteristic.h>
#include <BLERemoteService.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

using onNotifyCallback = std::function<void(const uint8_t* data, size_t len)>;

class BLEClient : public BLEClientCallbacks {
public:
    BLEClient();

    bool init();
    bool connectToDevice();
    void disconnect();
    bool write(const uint8_t* data, size_t len);
    void registerNotifyCallback(onNotifyCallback callback);

    bool isConnected() const { return _connected; }
    bool isTransparent() const { return _transparent; }
    void setTransparent(bool transparent);

    void onConnect(BLEClient* pClient) override;
    void onDisconnect(BLEClient* pClient) override;
    void onServiceDiscoverComplete(BLERemoteService* pRemoteService) override;
    void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void handleServiceDiscover(BLERemoteService* pRemoteService);

    static BLEClient* getInstance();

private:
    struct ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
        void onResult(BLEAdvertisedDevice* pDevice) override;
    };

    bool _connected = false;
    bool _transparent = false;
    onNotifyCallback _notifyCallback;
    ScanCallbacks _scanCallbacks;

    BLEAdvertisedDevice* _pDevice = nullptr;
    BLEClient* _pClient = nullptr;
    BLERemoteCharacteristic* _pWriteChar = nullptr;
    BLERemoteCharacteristic* _pNotifyChar = nullptr;
};

#endif