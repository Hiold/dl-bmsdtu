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
    Serial.print("[BOOT] Target: ");
    Serial.println(TARGET_NAME);

    uart.begin(UART_BAUD);
    bleGateway.init();
    bleGateway.startScan();

    bleGateway.registerNotifyCallback([](const uint8_t* data, size_t len) {
        uart.write(data, len);
    });

    stateMachine.setState(State::SCANNING);
    Serial.println("[BOOT] Ready\n");
}

void loop() {
    uint32_t now = millis();
    State state = stateMachine.getState();

    static State lastState = State::IDLE;
    if (state != lastState) {
        Serial.print("[STATE] ");
        Serial.println(stateMachine.stateToString());
        lastState = state;
    }

    static uint32_t lastScanRestart = 0;

    switch (state) {
    case State::SCANNING:
    case State::CONNECTING:
    case State::CONNECTED:
        if (bleGateway.isTransparent()) {
            stateMachine.setState(State::TRANSPARENT);
            Serial.println("[BLE] Transparent mode active!");
        } else if (now - lastScanRestart > 35000) {
            Serial.println("[BLE] Scan timeout, restarting...");
            bleGateway.startScan();
            lastScanRestart = now;
        }
        break;

    case State::TRANSPARENT:
        {
            uint8_t buf[256];
            if (uart.available() > 0) {
                size_t len = uart.read(buf, sizeof(buf));
                bleGateway.write(buf, len);
            }
        }
        break;

    case State::ERROR:
        if (stateMachine.shouldRetry(now)) {
            stateMachine.setState(State::SCANNING);
            bleGateway.startScan();
            lastScanRestart = now;
        }
        break;
    }

    delay(10);
}
