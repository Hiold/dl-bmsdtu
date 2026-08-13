# Serial1-BLE 数据透传实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在 ESP32-C3 上实现 Serial1 与 BLE 双向数据透传，不改变现有 BLE 发现/连接逻辑

**架构：** 主循环轮询 Serial1 数据并同步写入 BLE 特征；BLE Notification 回调中透传数据到 Serial1

**技术栈：** Arduino Framework, NimBLE-Arduino, PlatformIO

---

## 文件结构

- 修改：`src/main.cpp` - 唯一修改文件

---

## 任务 1：添加 UARTHandler 全局实例并初始化

**文件：**
- 修改：`src/main.cpp:1-20` (添加 include 和全局实例)
- 修改：`src/main.cpp:305-310` (setup 中初始化 uart)

- [ ] **步骤 1：添加 UARTHandler include**

在 `#include <config.h>` 后添加：
```cpp
#include "uart/UARTHandler.h"
```

- [ ] **步骤 2：添加 UARTHandler 全局实例**

在 `static bool doConnect = false;` 后添加：
```cpp
UARTHandler uart;
```

- [ ] **步骤 3：在 setup() 中初始化 UART**

在 `Serial.begin(115200);` 后、`Serial.printf("Starting NimBLE Client\n");` 前添加：
```cpp
uart.begin(UART_BAUD);
```

- [ ] **步骤 4：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功，无错误

---

## 任务 2：实现 Serial1→BLE 透传

**文件：**
- 修改：`src/main.cpp:352-373` (loop 函数)

- [ ] **步骤 1：在 loop() 中添加 Serial1 轮询和 BLE 写入**

在 `loop()` 函数 `delay(10);` 后、`if (doConnect)` 前添加：
```cpp
    const size_t BRIDGE_BUF_SIZE = 512;
    static uint8_t bridgeBuf[BRIDGE_BUF_SIZE];
    size_t uartAvail = uart.available();
    if (uartAvail > 0) {
        size_t readLen = min(uartAvail, BRIDGE_BUF_SIZE);
        size_t actualLen = uart.read(bridgeBuf, readLen);
        if (actualLen > 0 && pChr && pChr->canWrite()) {
            pChr->writeValue(bridgeBuf, actualLen);
        }
    }
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 3：实现 BLE→Serial1 透传

**文件：**
- 修改：`src/main.cpp:91-101` (notifyCB 函数)

- [ ] **步骤 1：在 notifyCB 末尾添加 Serial1 写入**

将 `notifyCB` 修改为：
```cpp
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str += ", Value = " + std::string((char *)pData, length);
    Serial.printf("%s\n", str.c_str());

    uart.write(pData, length);
}
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 4：整体验证

- [ ] **步骤 1：完整编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功，生成 `.pio/build/esp32c3supermini/firmware.bin`

- [ ] **步骤 2：检查修改内容**

运行：`git diff src/main.cpp`
确认修改符合设计

- [ ] **步骤 3：Commit**

```bash
git add src/main.cpp
git commit -m "feat: add serial1-ble bidirectional data bridge"
```
