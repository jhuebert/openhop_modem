// =============================================================
// boards/rak3401.h — RAKwireless RAK3401 openHop Modem
//
// Target hardware:
//   * RAK3401: nRF52840 WisBlock core + RAK13302 LoRa module
//     (Semtech SX1262 + Skyworks SKY66122-11 front end, ~1 W)
//   * Any compatible WisBlock base with native USB connected
//
// Pin map and RF policy ported from the MeshCore `rak3401` variant.
// Unlike the RAK4631 targets here the SX1262 is not the in-module
// radio: it lives on the WisBlock SPI slot, so the variant's default
// SPI bus (P0.03/P0.29/P0.30) is the LoRa bus and main.cpp uses the
// generic `SPI.begin()` path.
//
// Front-end control:
//   * WB_IO2 (P0.34) gates the 3V3_S rail AND the RAK13302's 5 V
//     boost for the PA — held HIGH for the life of the device.
//   * WB_IO3 (P0.21) drives the SKY66122 CSD+CPS pair (tied on the
//     PCB) — HIGH enables the FEM (LNA on RX, PA path for TX).
//   * CTX is wired to SX1262 DIO2, so TX/RX switching is timed by
//     the radio in hardware; no MCU-driven rx_pin/tx_pin needed.
//
// USB-CDC is the only modem transport: the nRF52840 has BLE but no
// Wi-Fi, and this target ships no Ethernet module.
//
// nRF52 GPIO numbering follows the repo's identity variant convention:
// raw pin number N maps to Arduino pin N. P1.x is encoded as 32 + x.
// =============================================================
#pragma once

#ifndef OPENHOP_RAK3401_3V3_EN
#  define OPENHOP_RAK3401_3V3_EN 34  // P0.34 / WB_IO2 — 3V3_S + 5 V PA boost
#endif

#ifndef OPENHOP_RAK3401_FEM_EN
#  define OPENHOP_RAK3401_FEM_EN 21  // P0.21 / WB_IO3 — SKY66122 CSD+CPS
#endif

inline const BoardConfig BOARD = {
    "RAK3401",
    "rak3401",
    "openhop-rak3401",

    // SX1262 on the WisBlock SPI slot (RAK13302).
    26,  // pin_lora_nss  = P0.26 (WB_SPI_CS)
    4,   // pin_lora_rst  = P0.04 (WB_IO4)
    9,   // pin_lora_busy = P0.09 (WB_IO5)
    10,  // pin_lora_dio1 = P0.10 (WB_IO6)
    3,   // pin_lora_sck  = P0.03 (WB_SPI_CLK)
    29,  // pin_lora_miso = P0.29 (WB_SPI_MISO)
    30,  // pin_lora_mosi = P0.30 (WB_SPI_MOSI)

    // SKY66122 CTX hangs off SX1262 DIO2 — the radio switches TX/RX
    // itself. The FEM enable is a fixed level, so it belongs in
    // static_gpios below rather than in the toggled rx/tx pins.
    {-1, 0, -1, -1, true},

    36,    // pin_lora_tx_led = P1.04 / RAK19007 blue user LED
    true,  // lora_tx_led_active_high

    // No display in this firmware target (oled_display.cpp is not
    // compiled for nRF52 builds).
    -1,  // pin_i2c_sda
    -1,  // pin_i2c_scl
    -1,  // pin_i2c_oled_rst
    -1,  // pin_vext_enable_low

    -1,    // pin_tft_sda
    -1,    // pin_tft_scl
    -1,    // pin_tft_dc
    -1,    // pin_tft_rst
    -1,    // pin_tft_cs
    -1,    // pin_tft_bl
    true,  // tft_bl_active_high
    -1,    // pin_tft_power
    true,  // tft_power_active_high

    -1,    // pin_user_button — WisBlock base has reset only, no PRG GPIO
    true,  // user_button_active_low

    // LiPo divider tap on P0.05 (A0 / AIN3), 1.73:1 ratio. The nRF52
    // shim in compat.h samples against the BSP's default 3.6 V full
    // scale, so the raw pin millivolts only need the divider ratio.
    {5, -1, true, 1.73f},

    22,  // max_tx_power_dbm — SX1262 drive into the SKY66122 PA

    true,  // use_dio3_tcxo
    1.8f,  // tcxo_voltage

    140,   // sx126x_current_limit_ma — SX1262 PA_BOOST + FEM path
    true,  // sx126x_rx_boosted_gain — improve weak-packet receive margin
    true,  // sx126x_register_patch — 0x08B5 patch, better RX with the FEM

    true,   // has_lora_radio
    false,  // has_wifi — nRF52840 has BLE but no Wi-Fi
    {},     // wifi_antenna_switch
    false,  // has_network — USB-CDC transport only

    // No dedicated UART: the openHop protocol runs on native USB-CDC Serial.
    -1,      // pin_protocol_uart_rx
    -1,      // pin_protocol_uart_tx
    921600,  // protocol_uart_baud

    // RAK1910 GNSS on the WisBlock base is an optional add-on and is not
    // assumed present. Wire it to UART1 (P0.15 RX / P0.16 TX) and set
    // these to 15 / 16 to enable it.
    -1,     // pin_gps_uart_rx
    -1,     // pin_gps_uart_tx
    9600,   // gps_uart_baud
    -1,     // pin_gps_enable
    true,   // gps_enable_active_high
    -1,     // pin_gps_reset
    false,  // gps_reset_active_high
    true,   // gps_send_casic_config

    // No Ethernet PHY or W5100S transport on this target.
    {false, BoardConfig::EthernetPhy::NONE, -1, -1, -1, -1, false, false,
     {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},

    {},  // pmu — no PMU chip on this board

    // Order matters: raise the 3V3_S / 5 V boost rail first, then enable
    // the SKY66122. initVariant() already drives WB_IO2 high at boot, so
    // the PA supply has settled well before the FEM comes up here.
    {{OPENHOP_RAK3401_3V3_EN, true}, {OPENHOP_RAK3401_FEM_EN, true},
     {-1, false}, {-1, false}},
    2,
};
