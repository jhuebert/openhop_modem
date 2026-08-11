// =============================================================
// pmu_manager.h — AXP2101 PMU bring-up (LilyGO T-Beam-S3 Supreme)
//
// Some carriers gate the LoRa/GNSS/OLED power rails through a PMU chip
// instead of plain GPIO enables. begin() is a no-op on every board that
// leaves BOARD.pmu.enabled at its default false.
// =============================================================
#pragma once

namespace PmuManager {

// Must run before OledDisplay::begin(), SPI.begin()/radio.begin(), and
// GPSManager::begin() on boards where BOARD.pmu.enabled is true — those
// rails aren't powered until this returns. Returns false (non-fatal; logs
// a warning) if the PMU chip doesn't answer on its I2C bus.
bool begin();

} // namespace PmuManager
