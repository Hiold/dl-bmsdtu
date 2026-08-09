#include <Arduino.h>
#include "config.h"
#include "statemachine/StateMachine.h"
#include "uart/UARTHandler.h"
#include "ble/BLEGateway.h"

UARTHandler uart;
BLEGateway bleGateway;
StateMachine stateMachine;

void setup() {
    Serial.begin(UART_BAUD);
    delay(100);

    bleGateway.init();

    bleGateway.registerNotifyCallback([](const uint8_t* data, size_t len) {
        uart.write(data, len);
    });

    stateMachine.setState(State::SCANNING);
    bleGateway.connectToDevice();
}

void loop() {
    uint32_t now = millis();
    State state = stateMachine.getState();

    switch (state) {
    case State::SCANNING:
    case State::CONNECTING:
    case State::CONNECTED:
        if (bleGateway.isTransparent()) {
            stateMachine.setState(State::TRANSPARENT);
        }
        break;

    case State::TRANSPARENT:
        {
            uint8_t buf[256];
            if (uart.available() > 0) {
                size_t readLen = uart.read(buf, sizeof(buf));
                bleGateway.write(buf, readLen);
            }
        }
        break;

    case State::ERROR:
        if (stateMachine.shouldRetry(now)) {
            stateMachine.setState(State::SCANNING);
            stateMachine.recordRetryTime(now);
            bleGateway.connectToDevice();
        }
        break;
    }

    delay(10);
}
