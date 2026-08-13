
/** NimBLE_Client Demo:
 *
 *  Demonstrates many of the available features of the NimBLE client library.
 *
 *  Created: on March 24 2020
 *      Author: H2zero
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <config.h>
#include "uart/UARTHandler.h"

static bool isConnect = false;
UARTHandler uart;
static uint32_t scanTimeMs = 5000; /** scan time in milliseconds, 0 = scan forever */
static NimBLERemoteCharacteristic *pChr = nullptr;
static NimBLERemoteCharacteristic *pChr2 = nullptr;

// const char *TARGET_MAC = "6c:09:3b:8a:b6:5c";

size_t hexStrToBytes(const char *hexStr, uint8_t *bytes, size_t maxLen)
{
  size_t len = strlen(hexStr);
  if (len % 2 != 0 || len / 2 > maxLen)
    return 0;

  for (size_t i = 0; i < len / 2; i++)
  {
    char high = hexStr[i * 2];
    char low = hexStr[i * 2 + 1];
    if (!isxdigit(high) || !isxdigit(low))
      return 0;

    uint8_t h = isdigit(high) ? high - '0' : toupper(high) - 'A' + 10;
    uint8_t l = isdigit(low) ? low - '0' : toupper(low) - 'A' + 10;
    bytes[i] = (h << 4) | l;
  }
  return len / 2;
}

void bytesToHexStr(const uint8_t *bytes, size_t len, char *hexStr)
{
  static const char hexChars[] = "0123456789ABCDEF";
  for (size_t i = 0; i < len; i++)
  {
    hexStr[i * 2] = hexChars[bytes[i] >> 4];
    hexStr[i * 2 + 1] = hexChars[bytes[i] & 0x0F];
  }
  hexStr[len * 2] = '\0';
}

class ClientCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient) override { Serial.printf("Connected\n"); }

  void onDisconnect(NimBLEClient *pClient, int reason) override
  {
    Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
    isConnect = false;
    NimBLEDevice::getScan()->start(scanTimeMs, false, true);
  }
} clientCallbacks;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";
  str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
  str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
  str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
  str += ", Value = " + std::string((char *)pData, length);
  Serial.printf("%s\n", str.c_str());

  Serial.printf("BLE recv hex: ");
  for (size_t i = 0; i < length; i++)
  {
    Serial.printf("%02X", pData[i]);
  }
  Serial.printf("\n");

  if (length > 0)
  {
    char hexStr[length * 2 + 1];
    bytesToHexStr(pData, length, hexStr);
    Serial.printf("Converted hexStr: %s\n", hexStr);
    uart.write((uint8_t *)hexStr, strlen(hexStr));
  }
}

bool connectToServer()
{
  if (isConnect)
  {
    Serial.printf("Already Connected, Do Nothing");
    return true;
  }
  NimBLEClient *pClient = nullptr;
  if (NimBLEDevice::getCreatedClientCount())
  {
    pClient = NimBLEDevice::createClient();
    if (pClient)
    {
      if (!pClient->connect(NimBLEAddress(TARGET_MAC, BLE_ADDR_RANDOM), false))
      {
        Serial.printf("Reconnect failed\n");
        return false;
      }
      Serial.printf("Reconnected client\n");
    }
    else
    {
      /**
       *  We don't already have a client that knows this device,
       *  check for a client that is disconnected that we can use.
       */
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }

  /** No client to reuse? Create a new one. */
  if (!pClient)
  {
    if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS)
    {
      Serial.printf("Max clients reached - no more connections available\n");
      return false;
    }

    pClient = NimBLEDevice::createClient();

    Serial.printf("New client created\n");

    pClient->setClientCallbacks(&clientCallbacks, false);
    /**
     *  Set initial connection parameters:
     *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
     *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
     *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
     */
    pClient->setConnectionParams(200, 500, 0, 150);

    /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
    pClient->setConnectTimeout(5 * 1000);

    if (!pClient->connect(NimBLEAddress(TARGET_MAC, BLE_ADDR_RANDOM)))
    {
      /** Created a client but failed to connect, don't need to keep it as it has no data */
      NimBLEDevice::deleteClient(pClient);
      Serial.printf("Failed to connect, deleted client\n");
      return false;
    }
  }

  if (!pClient->isConnected())
  {
    if (!pClient->connect(NimBLEAddress(TARGET_MAC, BLE_ADDR_RANDOM)))
    {
      Serial.printf("Failed to connect\n");
      return false;
    }
  }

  pClient->setConnectionParams(12, 12, 0, 150);

  Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

  // 服务指针
  NimBLERemoteService *pSvc = nullptr;
  // 特性指针
  NimBLERemoteDescriptor *pDsc = nullptr;
  //
  pSvc = pClient->getService(SERVICE_UUID);
  if (pSvc)
  {
    pChr = pSvc->getCharacteristic(CHAR_WRITE_UUID);
    pChr2 = pSvc->getCharacteristic(CHAR_NOTIFY_UUID);
  }
  else
  {
    Serial.printf("Service FFE0 Not Found\n");
    return false;
  }
  // 核对写入目标特征
  if (pChr)
  {
    if (pChr->canWrite())
    {
      Serial.printf("Found Writeble Descriptor FFE1\n");
    }
    else
    {
      Serial.printf("Descriptor FFE1 Not Writeble\n");
      return false;
    }
  }
  else
  {
    Serial.printf("Found Writeble Descriptor FFE1\n");
    return false;
  }
  // 核对通知目标特征
  if (pChr2)
  {
    if (pChr2->canNotify() || pChr2->canIndicate())
    {
      if (!pChr2->subscribe(true, notifyCB))
      {
        pClient->disconnect();
        return false;
      }
      else
      {
        Serial.printf("Found subscribe Descriptor FFE2\n");
      }
    }
    else
    {
      Serial.printf("Descriptor FFE2 Not subscribeble\n");
      return false;
    }
  }
  else
  {
    Serial.printf("Can not Found subscribeble Descriptor FFE2\n");
    return false;
  }
  Serial.printf("All Done with this device!\n");
  return true;
}

void connect(void *arg)
{
  while (1)
  {
    isConnect = connectToServer();
    if (!isConnect)
    {
      Serial.printf("Can Not Find Target Device sleep 30s\n");
      // esp_sleep_enable_timer_wakeup(30 * 1000000);
      // esp_light_sleep_start();
      // initSerial();
      // Serial.println("Woke up by timer!");
      vTaskDelay(pdMS_TO_TICKS(30 * 1000));
    }
    else
    {
      // 延迟30秒
      vTaskDelay(pdMS_TO_TICKS(30 * 1000));
    }
  }
}

void scanSerialInput(void *arg)
{
  while (1)
  {
    const size_t BRIDGE_BUF_SIZE = 256;
    static uint8_t bridgeBuf[BRIDGE_BUF_SIZE];
    static char hexStrBuf[BRIDGE_BUF_SIZE * 2 + 1];
    size_t uartAvail = uart.available();
    if (uartAvail > 0)
    {
      size_t readLen = min(uartAvail, (size_t)(BRIDGE_BUF_SIZE * 2));
      size_t actualLen = uart.read((uint8_t *)hexStrBuf, readLen);
      hexStrBuf[actualLen] = '\0';
      size_t byteLen = hexStrToBytes(hexStrBuf, bridgeBuf, BRIDGE_BUF_SIZE);
      if (byteLen > 0 && pChr && pChr->canWrite())
      {
        pChr->writeValue((uint8_t *)bridgeBuf, byteLen);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void hearbeat(void *arg)
{
  while (1)
  {
    if (isConnect)
    {
      // 延迟30秒
      if (pChr)
      {
        if (pChr->canWrite())
        {
          pChr->writeValue("Tasty");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10 * 1000));
  }
}

void setup()
{
  Serial.begin(115200);
  uart.begin(UART_BAUD);
  delay(500);
  Serial.println("Serial Initialzied");
  Serial.printf("Starting NimBLE Client\n");
  /** Initialize NimBLE and set the device name */
  NimBLEDevice::init("NimBLE-Client");
  NimBLEDevice::setPower(0); /** 3dbm */
  // 关闭电源指示灯

  // // 4. 全局设置：允许 Modem Sleep（ESP32-C3 默认支持）
  // esp_pm_config_esp32c3_t pm_config = {
  //     .max_freq_mhz = 80,        // CPU 最大频率（降低到80MHz）
  //     .min_freq_mhz = 40,        // CPU 最小频率
  //     .light_sleep_enable = true // 允许浅睡眠（Modem Sleep）
  // };
  // esp_pm_configure(&pm_config);

  // 创建链接蓝牙任务
  xTaskCreate(connect, "connect", 2048, NULL, 1, NULL);
  xTaskCreate(scanSerialInput, "light_flash", 2048, NULL, 1, NULL);
  xTaskCreate(hearbeat, "hearbeat", 2048, NULL, 1, NULL);
}

// 不使用loop
void loop()
{
}