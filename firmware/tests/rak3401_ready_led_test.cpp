#include "rak3401_ready_led.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

void testSuccessfulInitProducesBriefPulseThenTurnsOff() {
    Rak3401ReadyLed heartbeat;

    heartbeat.begin(1000U);

    assert(heartbeat.isLit());
    assert(!heartbeat.update(1049U));
    assert(heartbeat.isLit());
    assert(heartbeat.update(1050U));
    assert(!heartbeat.isLit());
}

void testHeartbeatPulsesOncePerMinute() {
    Rak3401ReadyLed heartbeat;

    heartbeat.begin(1000U);
    heartbeat.update(1050U);

    assert(!heartbeat.update(60999U));
    assert(!heartbeat.isLit());
    assert(heartbeat.update(61000U));
    assert(heartbeat.isLit());
    assert(!heartbeat.update(61049U));
    assert(heartbeat.isLit());
    assert(heartbeat.update(61050U));
    assert(!heartbeat.isLit());
}

void testTimerWorksAcrossMillisWraparound() {
    Rak3401ReadyLed heartbeat;
    constexpr uint32_t start = UINT32_MAX - 24U;

    heartbeat.begin(start);

    assert(heartbeat.update(start + Rak3401ReadyLed::PULSE_DURATION_MS));
    assert(!heartbeat.isLit());
    assert(!heartbeat.update(start + Rak3401ReadyLed::HEARTBEAT_INTERVAL_MS - 1U));
    assert(heartbeat.update(start + Rak3401ReadyLed::HEARTBEAT_INTERVAL_MS));
    assert(heartbeat.isLit());
}

}  // namespace

int main() {
    testSuccessfulInitProducesBriefPulseThenTurnsOff();
    testHeartbeatPulsesOncePerMinute();
    testTimerWorksAcrossMillisWraparound();
    std::cout << "RAK3401 ready LED heartbeat tests passed\n";
    return 0;
}
