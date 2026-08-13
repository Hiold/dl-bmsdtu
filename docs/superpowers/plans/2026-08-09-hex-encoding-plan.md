# HEX 编码转换实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在 Serial1-BLE 透传中添加 HEX 编码转换：DTU发送字符HEX字符串转为真实HEX发送给设备，设备数据转为字符HEX字符串发回DTU

**架构：** 添加 hexStrToBytes 和 bytesToHexStr 辅助函数，修改透传逻辑使用这些函数进行编码转换

**技术栈：** Arduino Framework, NimBLE-Arduino, PlatformIO

---

## 文件结构

- 修改：`src/main.cpp` - 唯一修改文件

---

## 任务 1：添加 hexStrToBytes 函数

**文件：**
- 修改：`src/main.cpp`（在全局变量声明区域后、类定义前添加）

- [ ] **步骤 1：添加 hexStrToBytes 函数**

在全局变量 `static NimBLERemoteCharacteristic *pChr = nullptr;` 后添加：

```cpp
size_t hexStrToBytes(const char* hexStr, uint8_t* bytes, size_t maxLen) {
    size_t len = strlen(hexStr);
    if (len % 2 != 0 || len / 2 > maxLen) return 0;

    for (size_t i = 0; i < len / 2; i++) {
        char high = hexStr[i * 2];
        char low = hexStr[i * 2 + 1];
        if (!isxdigit(high) || !isxdigit(low)) return 0;

        uint8_t h = isdigit(high) ? high - '0' : toupper(high) - 'A' + 10;
        uint8_t l = isdigit(low) ? low - '0' : toupper(low) - 'A' + 10;
        bytes[i] = (h << 4) | l;
    }
    return len / 2;
}
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 2：添加 bytesToHexStr 函数

**文件：**
- 修改：`src/main.cpp`（在 hexStrToBytes 函数后添加）

- [ ] **步骤 1：添加 bytesToHexStr 函数**

在 hexStrToBytes 函数后添加：

```cpp
void bytesToHexStr(const uint8_t* bytes, size_t len, char* hexStr) {
    static const char hexChars[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        hexStr[i * 2] = hexChars[bytes[i] >> 4];
        hexStr[i * 2 + 1] = hexChars[bytes[i] & 0x0F];
    }
    hexStr[len * 2] = '\0';
}
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 3：修改 loop() 中的 Serial1→BLE 透传

**文件：**
- 修改：`src/main.cpp:355-380` (loop 函数中的透传逻辑)

- [ ] **步骤 1：修改 loop() 中的 Serial1→BLE 透传**

将现有的：
```cpp
    const size_t BRIDGE_BUF_SIZE = 512;
    static uint8_t bridgeBuf[BRIDGE_BUF_SIZE];
    size_t uartAvail = uart.available();
    if (uartAvail > 0) {
        size_t readLen = min(uartAvail, BRIDGE_BUF_SIZE);
        size_t actualLen = uart.read(bridgeBuf, readLen);
        if (actualLen > 0 && pChr && pChr->canWrite()) {
            pChr->writeValue((uint8_t*)bridgeBuf, actualLen);
        }
    }
```

替换为：
```cpp
    const size_t BRIDGE_BUF_SIZE = 256;
    static uint8_t bridgeBuf[BRIDGE_BUF_SIZE];
    static char hexStrBuf[BRIDGE_BUF_SIZE * 2 + 1];
    size_t uartAvail = uart.available();
    if (uartAvail > 0) {
        size_t readLen = min(uartAvail, (int)(BRIDGE_BUF_SIZE * 2));
        size_t actualLen = uart.read((uint8_t*)hexStrBuf, readLen);
        hexStrBuf[actualLen] = '\0';
        size_t byteLen = hexStrToBytes(hexStrBuf, bridgeBuf, BRIDGE_BUF_SIZE);
        if (byteLen > 0 && pChr && pChr->canWrite()) {
            pChr->writeValue(bridgeBuf, byteLen);
        }
    }
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 4：修改 notifyCB 中的 BLE→Serial1 透传

**文件：**
- 修改：`src/main.cpp:93-105` (notifyCB 函数)

- [ ] **步骤 1：修改 notifyCB 中的 BLE→Serial1 透传**

在 `Serial.printf("%s\n", str.c_str());` 后、`}` 之前添加：

```cpp
    if (length > 0) {
        char hexStr[length * 2 + 1];
        bytesToHexStr(pData, length, hexStr);
        uart.write((uint8_t*)hexStr, strlen(hexStr));
    }
```

- [ ] **步骤 2：验证编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

---

## 任务 5：整体验证

- [ ] **步骤 1：完整编译**

运行：`pio run -e esp32c3supermini`
预期：编译成功

- [ ] **步骤 2：检查修改内容**

运行：`git diff src/main.cpp`
确认修改包含 hexStrToBytes、bytesToHexStr 函数和两处透传逻辑的修改

- [ ] **步骤 3：Commit**

```bash
git add src/main.cpp
git commit -m "feat: add hex encoding conversion for serial1-ble bridge"
```
