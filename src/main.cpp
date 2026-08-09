#include <Arduino.h>
#include "config.h"
#include "statemachine/StateMachine.h"
#include "uart/UARTHandler.h"
#include "ble/BLEClient.h"

UARTHandler uart;
BLEClient bleClient;
StateMachine stateMachine;

void setup() {
    Serial.begin(UART_BAUD);
    delay(100);

    bleClient.init();
    stateMachine.setState(State::SCANNING);
    bleClient.connectToDevice();
}

void loop() {
    uint32_t now = millis();
    State state = stateMachine.getState();

    switch (state) {
    case State::TRANSPARENT:
        {
            uint8_t buf[256];
            size_t len = uart.available();
            if (len > 0) {
                size_t readLen = uart.read(buf, sizeof(buf));
                bleClient.write(buf, readLen);
            }
        }
        break;

    case State::ERROR:
        if (stateMachine.shouldRetry(now)) {
            stateMachine.setState(State::SCANNING);
            stateMachine.recordRetryTime(now);
            bleClient.connectToDevice();
        }
        break;
    }

    delay(10);
}
