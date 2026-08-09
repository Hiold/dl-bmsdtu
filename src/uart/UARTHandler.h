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
