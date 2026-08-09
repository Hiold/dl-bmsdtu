#include "UARTHandler.h"

bool UARTHandler::begin(unsigned long baud) {
    _baud = baud;
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
