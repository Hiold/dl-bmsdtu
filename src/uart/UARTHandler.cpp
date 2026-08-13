#include "UARTHandler.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp32c3/rom/uart.h"

#define UART_PORT_NUM      UART_NUM_0  // 一般用 UART0 或 UART1
#define UART_RX_PIN        20          // 你的 RX 引脚
#define UART_TX_PIN        21          // 你的 TX 引脚（可省略）
#define UART_BAUD_RATE     115200

void setup_uart_wakeup(void) {
    // 1. 配置 UART 参数
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_PORT_NUM, 256, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // 2. 关键：设置 UART 为浅睡眠唤醒源
    uart_set_wakeup_threshold(UART_PORT_NUM, 3);  // 检测到 3 个边沿后唤醒（默认即可）
    
    // 3. 启用外设唤醒源（UART）
    esp_sleep_enable_uart_wakeup(UART_PORT_NUM);
    
    // 4. 可选：清除缓冲区，避免旧数据干扰
    uart_flush_input(UART_PORT_NUM);
}

bool UARTHandler::begin(unsigned long baud) {
    _baud = baud;
    setup_uart_wakeup();
    Serial1.begin(baud, SERIAL_8N1, 20, 21);
    delay(100);
    return true;
}

size_t UARTHandler::available() {
    return Serial1.available();
}

size_t UARTHandler::read(uint8_t* buf, size_t maxLen) {
    size_t count = 0;
    while (count < maxLen && Serial1.available()) {
        buf[count++] = Serial1.read();
    }
    return count;
}

size_t UARTHandler::write(const uint8_t* data, size_t len) {
    return Serial1.write(data, len);
}
