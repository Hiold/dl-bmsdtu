# HEX 编码转换设计

## 需求

在 Serial1-BLE 双向透传中增加 HEX 编码转换：

- DTU（Serial1）发送字符HEX字符串（如 "A1B2"），BLE 需转换为真实 HEX 再发送给设备
- BLE 收到设备数据（真实 HEX），需转换为字符HEX字符串再发送给 DTU

## 编码格式

- **字符HEX：** 连续字符串，如 "A1B2" 表示 0xA1, 0xB2
- **真实HEX：** 字节数组，如 `{0xA1, 0xB2}`

## 数据流

```
Serial1 RX ("A1B2")
    ↓
hexStrToBytes("A1B2") → {0xA1, 0xB2}
    ↓
pChr->writeValue({0xA1, 0xB2})
    ↓
BLE → 设备

设备 → BLE ({0xA1, 0xB2})
    ↓
bytesToHexStr({0xA1, 0xB2}) → "A1B2"
    ↓
uart.write("A1B2")
    ↓
Serial1 TX
```

## 实现

### 1. hexStrToBytes

将偶数长度 HEX 字符串转换为字节数组。

```cpp
size_t hexStrToBytes(const char* hexStr, uint8_t* bytes, size_t maxLen) {
    size_t len = strlen(hexStr);
    if (len % 2 != 0 || len / 2 > maxLen) return 0;

    for (size_t i = 0; i < len / 2; i++) {
        char high = hexStr[i * 2];
        char low = hexStr[i * 2 + 1];
        if (!isxdigit(high) || !isxdigit(low)) return 0;

        bytes[i] = (isdigit(high) ? high - '0' : toupper(high) - 'A' + 10) << 4 |
                   (isdigit(low) ? low - '0' : toupper(low) - 'A' + 10);
    }
    return len / 2;
}
```

### 2. bytesToHexStr

将字节数组转换为 HEX 字符串（无分隔符）。

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

### 3. 修改 loop() 中的 Serial1→BLE

```cpp
const size_t BRIDGE_BUF_SIZE = 512;
static uint8_t bridgeBuf[BRIDGE_BUF_SIZE];
static char hexStrBuf[BRIDGE_BUF_SIZE * 2 + 1];

size_t uartAvail = uart.available();
if (uartAvail > 0) {
    size_t readLen = min(uartAvail, BRIDGE_BUF_SIZE);
    size_t actualLen = uart.read((uint8_t*)hexStrBuf, readLen);
    hexStrBuf[actualLen] = '\0';

    size_t byteLen = hexStrToBytes(hexStrBuf, bridgeBuf, BRIDGE_BUF_SIZE);
    if (byteLen > 0 && pChr && pChr->canWrite()) {
        pChr->writeValue(bridgeBuf, byteLen);
    }
}
```

### 4. 修改 notifyCB 中的 BLE→Serial1

```cpp
void notifyCB(...) {
    // ... 现有日志代码 ...

    if (length > 0) {
        char hexStr[length * 2 + 1];
        bytesToHexStr(pData, length, hexStr);
        uart.write((uint8_t*)hexStr, strlen(hexStr));
    }
}
```

## 错误处理

- hexStrToBytes 输入不是有效 HEX：返回 0，跳过写入
- 输入长度奇数：返回 0，跳过写入
- BLE 写入失败：直接丢弃

## 修改文件

- `src/main.cpp`
