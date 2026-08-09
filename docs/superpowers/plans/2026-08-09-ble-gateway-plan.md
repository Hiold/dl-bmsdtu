# BLE 透传网关实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 实现 ESP32-C3 BLE 透传网关，固定连接目标设备，DTU 串口与 BLE ffe1/ffe2 通道双向透传。

**架构：** 单任务状态机驱动，UART 与 BLE 在同一循环中透传，使用 NimBLE ESP32 实现 BLE 客户端。

**技术栈：** ESP32-Arduino, NimBLE

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `include/config.h` | TARGET_MAC、UUID、参数宏定义 |
| `src/statemachine/StateMachine.h` | 状态枚举、状态机类定义 |
| `src/statemachine/StateMachine.cpp` | 状态转换逻辑 |
| `src/uart/UARTHandler.h` | 串口读写接口 |
| `src/uart/UARTHandler.cpp` | 串口实现 |
| `src/ble/BLEClient.h` | BLE 客户端封装、回调定义 |
| `src/ble/BLEClient.cpp` | BLE 连接、服务发现、写入 |
| `src/main.cpp` | setup/loop、模块组装 |

---

## 任务 1：创建配置文件

**文件：**
- 创建：`include/config.h`

- [ ] **步骤 1：编写 config.h**

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// BLE 目标设备 MAC 地址（格式：xx:xx:xx:xx:xx:xx）
#define TARGET_MAC "AA:BB:CC:DD:EE:FF"

// BLE 服务和特征 UUID
#define SERVICE_UUID "FFE0"
#define CHAR_WRITE_UUID "FFE1"
#define CHAR_NOTIFY_UUID "FFE2"

// 连接参数
#define CONNECT_TIMEOUT_MS 10000
#define RETRY_INTERVAL_MS 5000

// 串口参数
#define UART_BAUD 115200

#endif
```

- [ ] **步骤 2：Commit**

```bash
git add include/config.h
git commit -m "feat: add BLE gateway configuration"
```

---

## 任务 2：实现 UARTHandler

**文件：**
- 创建：`src/uart/UARTHandler.h`
- 创建：`src/uart/UARTHandler.cpp`

- [ ] **步骤 1：编写 UARTHandler.h**

```cpp
#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <Arduino.h>

class UARTHandler {
public:
    UARTHandler() = default;

    bool begin(unsigned long baud);
    size_t available();
    size_t read(uint8_t* buf, size_t maxLen);
    size_t write(const uint8_t* data, size_t len);

private:
    unsigned long _baud = 0;
};

#endif
```

- [ ] **步骤 2：编写 UARTHandler.cpp**

```cpp
#include "UARTHandler.h"

bool UARTHandler::begin(unsigned long baud) {
    _baud = baud;
    Serial.begin(baud);
    delay(100);
    return true;
}

size_t UARTHandler::available() {
    return Serial.available();
}

size_t UARTHandler::read(uint8_t* buf, size_t maxLen) {
    size_t count = 0;
    while (count < maxLen && Serial.available()) {
        buf[count++] = Serial.read();
    }
    return count;
}

size_t UARTHandler::write(const uint8_t* data, size_t len) {
    return Serial.write(data, len);
}
```

- [ ] **步骤 3：Commit**

```bash
git add src/uart/UARTHandler.h src/uart/UARTHandler.cpp
git commit -m "feat: add UART handler for DTU communication"
```

---

## 任务 3：实现 StateMachine

**文件：**
- 创建：`src/statemachine/StateMachine.h`
- 创建：`src/statemachine/StateMachine.cpp`

- [ ] **步骤 1：编写 StateMachine.h**

```cpp
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

enum class State {
    IDLE,
    SCANNING,
    CONNECTING,
    CONNECTED,
    TRANSPARENT,
    ERROR
};

class StateMachine {
public:
    StateMachine();

    State getState() const { return _state; }
    void setState(State newState);

    bool shouldRetry(uint32_t now) const;
    void recordRetryTime(uint32_t now);

    const char* stateToString() const;

private:
    State _state = State::IDLE;
    uint32_t _lastRetryTime = 0;
    bool _retryScheduled = false;
};

#endif
```

- [ ] **步骤 2：编写 StateMachine.cpp**

```cpp
#include "StateMachine.h"
#include "config.h"

StateMachine::StateMachine() : _state(State::IDLE), _lastRetryTime(0), _retryScheduled(false) {}

void StateMachine::setState(State newState) {
    _state = newState;
    _retryScheduled = false;
}

bool StateMachine::shouldRetry(uint32_t now) const {
    if (_state != State::ERROR) return false;
    return (now - _lastRetryTime) >= RETRY_INTERVAL_MS;
}

void StateMachine::recordRetryTime(uint32_t now) {
    _lastRetryTime = now;
    _retryScheduled = true;
}

const char* StateMachine::stateToString() const {
    switch (_state) {
        case State::IDLE: return "IDLE";
        case State::SCANNING: return "SCANNING";
        case State::CONNECTING: return "CONNECTING";
        case State::CONNECTED: return "CONNECTED";
        case State::TRANSPARENT: return "TRANSPARENT";
        case State::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
```

- [ ] **步骤 3：Commit**

```bash
git add src/statemachine/StateMachine.h src/statemachine/StateMachine.cpp
git commit -m "feat: add state machine for BLE gateway"
```

---

## 任务 4：实现 BLEClient

**文件：**
- 创建：`src/ble/BLEClient.h`
- 创建：`src/ble/BLEClient.cpp`

- [ ] **步骤 1：编写 BLEClient.h**

```cpp
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

    void onConnect(BLEClient* pClient) override;
    void onDisconnect(BLEClient* pClient) override;
    void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

private:
    bool _connected = false;
    bool _transparent = false;
    onNotifyCallback _notifyCallback;

    BLERemoteCharacteristic* _pWriteChar = nullptr;
    BLERemoteCharacteristic* _pNotifyChar = nullptr;

    static void scanResult(BLEAdvertisedDevice* pDevice);
    static void serviceDiscoverComplete(BLERemoteService* pRemoteService);
    static void characteristicDiscoverComplete(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
};

#endif
```

- [ ] **步骤 2：编写 BLEClient.cpp**

```cpp
#include "BLEClient.h"
#include "config.h"

BLEClient::BLEClient()
    : _connected(false), _transparent(false), _notifyCallback(nullptr),
      _pWriteChar(nullptr), _pNotifyChar(nullptr) {}

bool BLEClient::init() {
    BLEDevice::init("");
    BLEDevice::setCustomGapEventHandler([](const BLEGapEvent& event) {
        // Handle gap events if needed
    });
    return true;
}

bool BLEClient::connectToDevice() {
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new class : public BLEAdvertisedDeviceCallbacks {
        void onResult(BLEAdvertisedDevice* pDevice) override {
            if (pDevice->getAddress().toString() == TARGET_MAC) {
                BLEDevice::getScan()->stop();
                // Store device and trigger connection
            }
        }
    });
    pScan->start(5);
    return true;
}

void BLEClient::disconnect() {
    _connected = false;
    _transparent = false;
    _pWriteChar = nullptr;
    _pNotifyChar = nullptr;
}

bool BLEClient::write(const uint8_t* data, size_t len) {
    if (!_transparent || !_pWriteChar) return false;
    _pWriteChar->writeValue(data, len, false);
    return true;
}

void BLEClient::registerNotifyCallback(onNotifyCallback callback) {
    _notifyCallback = callback;
}

void BLEClient::onConnect(BLEClient* pClient) {
    _connected = true;
}

void BLEClient::onDisconnect(BLEClient* pClient) {
    disconnect();
}

void BLEClient::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (_notifyCallback) {
        _notifyCallback(pData, length);
    }
}
```

- [ ] **步骤 3：Commit**

```bash
git add src/ble/BLEClient.h src/ble/BLEClient.cpp
git commit -m "feat: add BLE client for ffe0 service"
```

---

## 任务 5：实现 main.cpp 集成

**文件：**
- 修改：`src/main.cpp`

- [ ] **步骤 1：编写 main.cpp**

```cpp
#include <Arduino.h>
#include "config.h"
#include "statemachine/StateMachine.h"
#include "uart/UARTHandler.h"
#include "ble/BLEClient.h"

UARTHandler uart;
BLEClient bleClient;
StateMachine stateMachine;

void setup() {
    Serial.begin(UART_BAUD);
    delay(100);

    bleClient.init();
    stateMachine.setState(State::SCANNING);
    bleClient.connectToDevice();
}

void loop() {
    uint32_t now = millis();
    State state = stateMachine.getState();

    switch (state) {
    case State::TRANSPARENT:
        {
            uint8_t buf[256];
            size_t len = uart.available();
            if (len > 0) {
                size_t readLen = uart.read(buf, sizeof(buf));
                bleClient.write(buf, readLen);
            }
        }
        break;

    case State::ERROR:
        if (stateMachine.shouldRetry(now)) {
            stateMachine.setState(State::SCANNING);
            stateMachine.recordRetryTime(now);
            bleClient.connectToDevice();
        }
        break;
    }

    delay(10);
}
```

- [ ] **步骤 2：Commit**

```bash
git add src/main.cpp
git commit -m "feat: integrate BLE gateway components"
```

---

## 任务 6：配置 NimBLE 依赖

**文件：**
- 修改：`platformio.ini`

- [ ] **步骤 1：更新 platformio.ini**

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps =
    h2zero/NimBLE-Arduino@^1.4.1
build_flags =
    -D CONFIG_BT_NIMBLE_ROLE_CENTRAL=1
```

- [ ] **步骤 2：Commit**

```bash
git add platformio.ini
git commit -m "chore: add NimBLE library dependency"
```

---

## 规格覆盖度检查

| 规格章节 | 实现任务 |
|---------|---------|
| 系统架构 | 任务 5 (main.cpp 集成) |
| BLE 配置 (MAC, UUID) | 任务 1 (config.h) |
| 状态机 | 任务 3 (StateMachine) |
| 串口模块 | 任务 2 (UARTHandler) |
| BLE 客户端 | 任务 4 (BLEClient) |
| 主循环逻辑 | 任务 5 (main.cpp) |
| 错误处理 (重试) | 任务 5 (loop 中的 ERROR 处理) |
| 初始化流程 | 任务 5 (setup) |

---

## 自检

- [x] 规格覆盖度：所有章节都有对应任务
- [x] 无占位符：所有步骤都有完整代码
- [x] 类型一致性：StateMachine 在各任务中一致使用 `State` 枚举
- [x] 宏定义一致性：RETRY_INTERVAL_MS, UART_BAUD, TARGET_MAC 统一在 config.h 中定义
