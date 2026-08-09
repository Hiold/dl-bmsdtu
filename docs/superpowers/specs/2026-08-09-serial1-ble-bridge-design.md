# Serial1-BLE 数据透传设计

## 需求

在保持现有 BLE 发现、连接逻辑完全不变的前提下，增加双向数据透传功能：

- Serial1 收到数据 → 通过 BLE 写入当前连接服务器的 FFE1 特征
- BLE 收到 Notification/Indication → 通过 Serial1 发送

## 架构

采用简单轮询模式，不引入额外线程或复杂状态机。

## 数据流

```
Serial1 RX (GPIO 20/21)
    ↓
UARTHandler::read() → buffer
    ↓
NimBLERemoteCharacteristic::writeValue()
    ↓
（失败则丢弃）

BLE Notification/Indication
    ↓
notifyCB() → UARTHandler::write()
    ↓
Serial1 TX
```

## 实现要点

### 1. 全局 UARTHandler 实例

```cpp
UARTHandler uart;
```

### 2. loop() 中的 Serial1→BLE 透传

```cpp
void loop() {
    // 现有连接逻辑保持不变
    if (doConnect) {
        doConnect = false;
        NimBLEDevice::getScan()->stop();
        if (connectToServer()) {
            Serial.printf("Success!\n");
        } else {
            Serial.printf("Failed to connect\n");
            delay(1000);
            NimBLEDevice::getScan()->start(scanTimeMs, false, true);
        }
    }

    // 新增：Serial1 → BLE 透传
    const size_t BUF_SIZE = 512;
    static uint8_t buf[BUF_SIZE];
    size_t len = uart.available();
    if (len > 0) {
        size_t readLen = min(len, BUF_SIZE);
        size_t actualLen = uart.read(buf, readLen);
        if (actualLen > 0 && pChr && pChr->canWrite()) {
            pChr->writeValue(buf, actualLen);
        }
    }

    delay(10);
}
```

### 3. notifyCB 中的 BLE→Serial1 透传

修改 `notifyCB`，将数据通过 UART 发送：

```cpp
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str += ", Value = " + std::string((char *)pData, length);
    Serial.printf("%s\n", str.c_str());

    // 新增：通过 Serial1 发送原始数据
    uart.write(pData, length);
}
```

### 4. setup() 中初始化 UART

```cpp
void setup() {
    Serial.begin(115200);
    uart.begin(UART_BAUD);  // 新增

    // ... 其余代码保持不变
}
```

## 错误处理

- BLE 写入失败：直接丢弃，不重试，不缓存
- Serial1 不可用（未连接）：数据丢失，但系统继续运行
- pChr 为空或不可写：跳过写入

## 修改文件

- `src/main.cpp`：唯一修改文件

## 验证方式

1. 打开 Arduino Serial Monitor (115200 baud)
2. 通过另一设备发送数据到 Neo10Pro → 观察 Serial1 输出
3. 向 Serial1 发送数据 → 观察 Neo10Pro 是否收到
