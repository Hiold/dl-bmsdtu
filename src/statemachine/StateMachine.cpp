#include "StateMachine.h"
#include "config.h"

StateMachine::StateMachine() : _state(State::IDLE), _lastRetryTime(0), _retryScheduled(false) {}

void StateMachine::setState(State newState) {
    _state = newState;
    _retryScheduled = false;
}

bool StateMachine::shouldRetry(uint32_t now) const {
    if (_state != State::ERROR) return false;
    return (now - _lastRetryTime) >= RETRY_INTERVAL_MS;
}

void StateMachine::recordRetryTime(uint32_t now) {
    _lastRetryTime = now;
    _retryScheduled = true;
}

const char* StateMachine::stateToString() const {
    switch (_state) {
        case State::IDLE: return "IDLE";
        case State::SCANNING: return "SCANNING";
        case State::CONNECTING: return "CONNECTING";
        case State::CONNECTED: return "CONNECTED";
        case State::TRANSPARENT: return "TRANSPARENT";
        case State::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}