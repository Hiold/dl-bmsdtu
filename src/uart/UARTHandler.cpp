#include "UARTHandler.h"

bool UARTHandler::begin(unsigned long baud) {
    _baud = baud;
    Serial.begin(baud);
    delay(100);
    return true;
}

size_t UARTHandler::available() {
    return Serial.available();
}

size_t UARTHandler::read(uint8_t* buf, size_t maxLen) {
    size_t count = 0;
    while (count < maxLen && Serial.available()) {
        buf[count++] = Serial.read();
    }
    return count;
}

size_t UARTHandler::write(const uint8_t* data, size_t len) {
    return Serial.write(data, len);
}
