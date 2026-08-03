#!/usr/bin/env python3
"""Focused source contract for Station G3-only PA mode control."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIRMWARE = ROOT / "firmware"

board_config = (FIRMWARE / "include/board_config.h").read_text()
station_g3 = (FIRMWARE / "include/boards/station_g3.h").read_text()
rf_header = (FIRMWARE / "include/rf_frontend.h").read_text()
rf_source = (FIRMWARE / "src/rf_frontend.cpp").read_text()
ota = (FIRMWARE / "src/ota_manager.cpp").read_text()

assert "bool   pa_user_selectable = false;" in board_config
assert ".pa_user_selectable = true" in station_g3

for board in (FIRMWARE / "include/boards").glob("*.h"):
    if board.name == "station_g3.h":
        continue
    assert ".pa_user_selectable = true" not in board.read_text(), board

for declaration in (
    "bool hasPaModeControl();",
    "bool isPaHighPowerEnabled();",
    "bool setPaHighPowerEnabled(bool enabled, bool persist);",
):
    assert declaration in rf_header

assert 'STATION_G3_PA_HIGH_KEY = "g3_pa_high"' in rf_source
assert "BOARD.rf_frontend.pa_user_selectable" in rf_source
assert "writeConfiguredLevel(BOARD.rf_frontend.pa_mode_pin" in rf_source
assert "p.getBool(STATION_G3_PA_HIGH_KEY, BOARD.rf_frontend.pa_default_high)" in rf_source
assert "p.putBool(STATION_G3_PA_HIGH_KEY, enabled)" in rf_source

begin_body = rf_source.split("void begin() {", 1)[1].split("bool hasPaModeControl()", 1)[0]
assert begin_body.index("writeConfiguredLevel(BOARD.rf_frontend.pa_mode_pin") < begin_body.index("p.getBool(STATION_G3_PA_HIGH_KEY")

set_pa_body = rf_source.split("bool setPaHighPowerEnabled", 1)[1].split("bool hasHeltecV43LnaControl", 1)[0]
assert set_pa_body.index("p.putBool(STATION_G3_PA_HIGH_KEY, enabled)") < set_pa_body.index("paHighPowerEnabled = enabled")

assert 'if (RFFrontEnd::hasPaModeControl()) {' in ota
assert "Station G3 RF Front-End" in ota
assert 'action=\'/rf-pa\'' in ota
route_guard = 'if (RFFrontEnd::hasPaModeControl()) {\n        httpServer->on("/rf-pa", HTTP_POST, handleRfPaSave);'
assert route_guard in ota
assert 'obj["pa_high_power_enabled"]' in ota
assert '"pa_high_power_enabled"' in ota
assert "PA mode control is not supported on this board" in ota

parse_body = ota.split("static bool applyConfigPatch", 1)[1].split("// ─── Rollback sanity check", 1)[0]
assert "RFFrontEnd::setPaHighPowerEnabled" not in parse_body
assert "rfPatch.paHighPowerEnabled = paVal.as<bool>();" in parse_body

post_body = ota.split("static void handleApiConfigPost()", 1)[1].split("static void handleApiReboot()", 1)[0]
assert post_body.index("applyConfigPatch") < post_body.index("RFFrontEnd::setPaHighPowerEnabled")

print("Station G3 PA control contract: PASS")
