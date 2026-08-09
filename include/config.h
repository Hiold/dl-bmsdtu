#ifndef CONFIG_H
#define CONFIG_H

// BLE 目标设备 MAC 地址（格式：xx:xx:xx:xx:xx:xx）
#define TARGET_MAC "75:de:9e:b4:f7:43"
#define TARGET_NAME "Neo10Pro"

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
