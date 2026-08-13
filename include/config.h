#ifndef CONFIG_H
#define CONFIG_H


#define TARGET_MAC "14:c1:9f:2c:58:b9"
#define LED_PIN 8

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
