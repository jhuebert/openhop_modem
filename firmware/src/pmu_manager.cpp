// =============================================================
// pmu_manager.cpp — AXP2101 PMU bring-up (LilyGO T-Beam-S3 Supreme)
//
// This file is compiled into every ESP32 env, but XPowersLib is only a
// lib_dep of lilygo_tbeam_s3_supreme (the only board with a PMU) — so the
// real implementation, and the <XPowersLib.h> include, is scoped to that
// board. Every other board gets a plain no-op.
// =============================================================
#include "pmu_manager.h"
#include "board_config.h"

#if defined(BOARD_LILYGO_TBEAM_S3_SUPREME)

#include <Wire.h>
#include <XPowersLib.h>

namespace {
XPowersLibInterface *pmu = nullptr;
}

bool PmuManager::begin() {
    if (!BOARD.pmu.enabled) {
        return true;
    }

    Wire1.begin(BOARD.pmu.i2c_sda, BOARD.pmu.i2c_scl);

    pmu = new XPowersAXP2101(Wire1);
    if (!pmu->init()) {
        Serial.println("[PMU] AXP2101 init failed — LoRa/GNSS/OLED rails will stay unpowered");
        delete pmu;
        pmu = nullptr;
        return false;
    }

    // Each rail is off at reset on this board; enable only what this
    // firmware profile needs (see PmuConfig in board_config.h).
    if (BOARD.pmu.aldo1) {
        pmu->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO1);
    }
    if (BOARD.pmu.aldo2) {
        pmu->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO2);
    }
    if (BOARD.pmu.aldo3) {
        pmu->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO3);
    }
    if (BOARD.pmu.aldo4) {
        pmu->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
        pmu->enablePowerOutput(XPOWERS_ALDO4);
    }

    Serial.println("[PMU] AXP2101 init ok, rails enabled");
    return true;
}

#else

bool PmuManager::begin() {
    return true;
}

#endif
