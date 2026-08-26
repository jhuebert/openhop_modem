#pragma once

#include <stdint.h>

// Nonblocking schedule for the RAK3401 green ready LED. A successful radio
// start produces one short pulse, followed by the same pulse once per minute.
class Rak3401ReadyLed {
public:
    static const uint32_t HEARTBEAT_INTERVAL_MS = 60000U;
    static const uint32_t PULSE_DURATION_MS = 50U;

    Rak3401ReadyLed()
        : enabled_(false), lit_(false), pulseStartedMs_(0U) {}

    void begin(uint32_t nowMs) {
        enabled_ = true;
        lit_ = true;
        pulseStartedMs_ = nowMs;
    }

    // Returns true only when the requested output state changes.
    bool update(uint32_t nowMs) {
        if (!enabled_) return false;

        const uint32_t elapsed = nowMs - pulseStartedMs_;
        if (lit_) {
            if (elapsed < PULSE_DURATION_MS) return false;
            lit_ = false;
            return true;
        }

        if (elapsed < HEARTBEAT_INTERVAL_MS) return false;
        lit_ = true;
        pulseStartedMs_ = nowMs;
        return true;
    }

    bool isLit() const { return lit_; }

private:
    bool enabled_;
    bool lit_;
    uint32_t pulseStartedMs_;
};
