# BLE 透传网关设计规格

**项目**：dl-bmsdtu — ESP32-C3 BLE 数据桥接器
**日期**：2026-08-09
**状态**：已批准

---

## 1. 系统概述

ESP32-C3 作为 BLE 与 DTU 串口之间的双向透传网关。固定连接一个 BLE 设备（服务 UUID 0xFFE0），将 DTU 串口数据转发至 BLE 写入通道（0xFFE1），将 BLE 通知通道（0xFFE2）的数据转发至 DTU 串口。

---

## 2. 系统架构

```
┌─────────────────────────────────────────┐
│              ESP32-C3                   │
│  ┌─────────┐    ┌──────────────────┐   │
│  │ UART0   │◄──►│   BLE Gateway     │   │
│  │(DTU)    │    │                  │   │
│  └─────────┘    └────────┬─────────┘   │
│                          │              │
│                   ffe1写入│ffe2通知     │
└──────────────────────────│──────────────┘
                           │
                    ┌──────┴──────┐
                    │  BLE Device │
                    └─────────────┘
```

---

## 3. 硬件配置

| 项目 | 配置 |
|------|------|
| 开发板 | ESP32-C3 DevKitM-1 |
| 串口 | UART0, 115200 8N1 |
| BLE | 内置 WiFi/BLE 复合芯片 |

---

## 4. BLE 配置

| 参数 | 值 |
|------|-----|
| 目标地址 | `TARGET_MAC` (编译时宏定义) |
| 服务 UUID | `0xFFE0` |
| 写入特征 | `0xFFE1` (write without response) |
| 通知特征 | `0xFFE2` (notify) |
| 连接超时 | 10 秒 |
| 重试间隔 | 5 秒 |

---

## 5. 状态机

```cpp
enum class State {
    IDLE,           // 初始态
    SCANNING,       // 扫描目标设备
    CONNECTING,     // 正在连接
    CONNECTED,      // 已连接，发现服务
    TRANSPARENT,    // 透传就绪
    ERROR           // 错误，等待重试
};
```

### 状态转换图

```
IDLE → SCANNING (setup)
SCANNING → CONNECTING (找到设备)
CONNECTING → CONNECTED (服务发现完成)
CONNECTING → ERROR (超时/失败)
CONNECTED → TRANSPARENT (ffe1/ffe2 就绪)
任何状态 → ERROR (断开)
ERROR → SCANNING (重试)
```

---

## 6. 模块接口

### BLE 模块

```cpp
bool BLE_init();
bool BLE_connectToDevice();
void BLE_disconnect();
bool BLE_write(const uint8_t* data, size_t len);
void BLE_registerCallback(onBLENotify callback);
```

### 串口模块

```cpp
bool UART_init(int baud);
size_t UART_read(uint8_t* buf, size_t maxLen);
void UART_write(const uint8_t* data, size_t len);
```

### 状态机

```cpp
void StateMachine_run();  // 主循环调用
State getCurrentState();
```

---

## 7. 主循环逻辑

```cpp
void loop() {
    static uint32_t lastRetry = 0;

    switch (state) {
    case TRANSPARENT:
        // 串口 → BLE
        if (uart.available()) {
            auto data = uart.read();
            BLE_write(data, dataLen);
        }
        // BLE → 串口 (回调中直接写入)
        break;

    case ERROR:
        if (millis() - lastRetry > RETRY_INTERVAL) {
            state = SCANNING;
            lastRetry = millis();
        }
        break;
    }
}
```

---

## 8. 错误处理

| 错误类型 | 处理方式 |
|---------|---------|
| 连接超时 | 切换 ERROR 状态，5 秒后重试 |
| 连接断开 | 立即切换 ERROR 状态，重试 |
| BLE 写入失败 | 记录日志，保持透传状态，等待恢复 |
| 串口错误 | 复位串口 |

---

## 9. 初始化流程

```
setup():
  1. Serial.begin(115200)
  2. BLE_init()
  3. State = SCANNING
  4. BLE_connectToDevice()  // 异步，状态机驱动
```

---

## 10. 文件结构

```
src/
  main.cpp              # 入口，状态机主循环
  ble/
    BLEClient.h         # BLE 客户端封装
    BLEClient.cpp
  uart/
    UARTHandler.h       # 串口处理
    UARTHandler.cpp
  statemachine/
    StateMachine.h      # 状态机定义
    StateMachine.cpp
include/
  config.h              # TARGET_MAC 等配置
lib/                    # 第三方库（如 NimBLE）
test/
  README
docs/superpowers/specs/
  2026-08-09-ble-gateway-design.md
```

---

## 11. 实现要点

- BLE 连接成功后，等待服务发现完成再切换到 TRANSPARENT
- ffe1 写入使用 write without response，不等待对端响应
- ffe2 通知在 GATT 回调中直接写入串口，无额外队列
- 重试使用 millis() 计时，不阻塞主循环
