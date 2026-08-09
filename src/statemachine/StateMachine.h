#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

enum class State {
    IDLE,
    SCANNING,
    CONNECTING,
    CONNECTED,
    TRANSPARENT,
    ERROR
};

class StateMachine {
public:
    StateMachine();

    State getState() const { return _state; }
    void setState(State newState);

    bool shouldRetry(uint32_t now) const;
    void recordRetryTime(uint32_t now);

    const char* stateToString() const;

private:
    State _state = State::IDLE;
    uint32_t _lastRetryTime = 0;
    bool _retryScheduled = false;
};

#endif