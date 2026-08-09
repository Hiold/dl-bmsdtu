#include <Arduino.h>
#include "config.h"
#include "statemachine/StateMachine.h"
#include "uart/UARTHandler.h"
#include "ble/BLEGateway.h"

UARTHandler uart;
BLEGateway bleGateway;
StateMachine stateMachine;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[BOOT] BLE Gateway starting...");
    Serial.print("[BOOT] Target MAC: ");
    Serial.println(TARGET_MAC);
    Serial.print("[BOOT] Target Service: ");
    Serial.println(SERVICE_UUID);

    uart.begin(UART_BAUD);
    Serial.println("[BOOT] UART1 initialized for DTU communication");

    bleGateway.init();

    bleGateway.registerNotifyCallback([](const uint8_t* data, size_t len) {
        uart.write(data, len);
        Serial.print("[UART] BLE->UART, len=");
        Serial.println(len);
    });

    stateMachine.setState(State::SCANNING);
    bleGateway.connectToDevice();
    Serial.println("[BOOT] BLE Gateway initialized, waiting for connection...\n");
}

void loop() {
    uint32_t now = millis();
    State state = stateMachine.getState();

    static State lastState = State::IDLE;
    if (state != lastState) {
        Serial.print("[STATE] -> ");
        Serial.println(stateMachine.stateToString());
        lastState = state;
    }

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
                Serial.print("[UART] UART->BLE, len=");
                Serial.println(readLen);
            }
        }
        break;

    case State::ERROR:
        if (stateMachine.shouldRetry(now)) {
            Serial.println("[RETRY] Attempting reconnection...");
            stateMachine.setState(State::SCANNING);
            stateMachine.recordRetryTime(now);
            bleGateway.connectToDevice();
        }
        break;
    }

    delay(10);
}
