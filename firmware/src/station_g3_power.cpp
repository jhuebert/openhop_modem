#include "station_g3_power.h"

#include <Arduino.h>

#if defined(BOARD_STATION_G3) && defined(ARDUINO_ARCH_ESP32)
#include <Adafruit_INA219.h>
#include <Wire.h>
#endif

namespace StationG3Power {

namespace {

static Snapshot current = {};

#if defined(BOARD_STATION_G3) && defined(ARDUINO_ARCH_ESP32)
static constexpr uint32_t SAMPLE_INTERVAL_MS = 250;
static constexpr uint8_t MAX_CONSECUTIVE_FAILURES = 3;
static constexpr uint32_t REPROBE_INTERVAL_MS = 30000;
static uint32_t lastSampleMs = 0;
static uint32_t lastProbeMs = 0;
static uint8_t consecutiveFailures = 0;
static Adafruit_INA219 ina219(0x40);

static bool sample() {
    float voltageV = ina219.getBusVoltage_V();
    bool voltageReadOk = ina219.success();
    float currentMa = ina219.getCurrent_mA();
    bool currentReadOk = ina219.success();
    if (!voltageReadOk || !currentReadOk || !isfinite(voltageV) || !isfinite(currentMa) ||
        voltageV < 0.0f || voltageV > 32.0f) {
        current.valid = false;
        if (++consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
            current.available = false;
            lastProbeMs = millis();
            Serial.println("[POWER] Station G3 INA219 offline; backing off for 30 seconds");
        }
        return false;
    }

    consecutiveFailures = 0;
    current.available = true;
    current.valid = true;
    current.inputVoltageV = voltageV;
    current.currentMa = currentMa;
    current.powerW = voltageV * currentMa / 1000.0f;
    if (current.minimumInputVoltageV == 0.0f || voltageV < current.minimumInputVoltageV) {
        current.minimumInputVoltageV = voltageV;
    }
    if (currentMa > current.maximumCurrentMa) {
        current.maximumCurrentMa = currentMa;
    }
    return true;
}

static bool tryBegin() {
    lastProbeMs = millis();
    if (!ina219.begin(&Wire)) {
        current.available = false;
        current.valid = false;
        return false;
    }

    // The Station G3 uses the INA219 reference 0.1-ohm shunt calibration;
    // BQ's Meshtastic configuration specifies the matching 1.0 multiplier.
    ina219.setCalibration_32V_2A();
    consecutiveFailures = 0;
    current.available = true;
    if (!sample()) {
        current.available = false;
        lastProbeMs = millis();
        return false;
    }
    lastSampleMs = millis();
    return true;
}
#endif

}  // namespace

void begin() {
#if defined(BOARD_STATION_G3) && defined(ARDUINO_ARCH_ESP32)
    Wire.setTimeOut(50);
    if (!tryBegin()) {
        Serial.println("[POWER] Station G3 INA219 not detected at 0x40");
        return;
    }
    Serial.println("[POWER] Station G3 INA219 ready");
#endif
}

void loop() {
#if defined(BOARD_STATION_G3) && defined(ARDUINO_ARCH_ESP32)
    uint32_t now = millis();
    if (!current.available) {
        if ((uint32_t)(now - lastProbeMs) < REPROBE_INTERVAL_MS) return;
        if (tryBegin()) {
            Serial.println("[POWER] Station G3 INA219 recovered");
        }
        return;
    }
    if ((uint32_t)(now - lastSampleMs) < SAMPLE_INTERVAL_MS) return;
    lastSampleMs = now;
    sample();
#endif
}

Snapshot snapshot() {
    return current;
}

}  // namespace StationG3Power
