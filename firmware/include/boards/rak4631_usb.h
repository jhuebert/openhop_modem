// =============================================================
// boards/rak4631_usb.h — RAK4631 USB-only openHop Modem
//
// Target hardware:
//   * RAK4631 Core Module: Nordic nRF52840 + Semtech SX1262
//   * Any compatible WisBlock base with native USB connected
//   * No RAK13800/W5100S Ethernet module required or initialized
//
// The internal SX1262 pin map and RF policy match the proven
// rak4631_wismesh_eth target. This variant intentionally compiles only the
// USB-CDC modem transport and leaves every network peripheral disabled.
//
// nRF52 GPIO numbering follows the repo's identity variant convention:
// raw pin number N maps to Arduino pin N. P1.x is encoded as 32 + x.
// =============================================================
#pragma once

#ifndef OPENHOP_RAK4631_SX126X_POWER_EN
#  define OPENHOP_RAK4631_SX126X_POWER_EN 37  // P1.05 — RAK4631 SX1262 power enable
#endif

inline const BoardConfig BOARD = {
    "RAK4631 USB",
    "rak4631_usb",
    "openhop-rakusb",

    // SX1262 internal LoRa bus/control pins.
    42,  // pin_lora_nss  = P1.10
    38,  // pin_lora_rst  = P1.06
    46,  // pin_lora_busy = P1.14
    47,  // pin_lora_dio1 = P1.15
    43,  // pin_lora_sck  = P1.11
    45,  // pin_lora_miso = P1.13
    44,  // pin_lora_mosi = P1.12

    // RAK4631 uses SX1262 DIO2 for the RF switch. RAK docs note that
    // P1.07/ANT_SW should not be initialized for custom SX1262 firmware.
    {-1, 0, -1, -1, true},

    -1,    // pin_lora_tx_led
    true,  // lora_tx_led_active_high

    // No display in this firmware target.
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

    -1,    // pin_user_button — base reset is hardware reset, no PRG GPIO
    true,  // user_button_active_low

    {-1, -1, true, 0.0f},  // no battery sense in this target

    22,  // max_tx_power_dbm — RAK4631/SX1262 PA_BOOST ceiling

    true,  // use_dio3_tcxo
    1.8f,  // tcxo_voltage

    140,   // sx126x_current_limit_ma — SX1262 PA_BOOST/RF switch path
    true,  // sx126x_rx_boosted_gain — improve weak-packet receive margin
    true,  // sx126x_register_patch — same SX126x sensitivity patch as tracker boards

    true,   // has_lora_radio
    false,  // has_wifi — nRF52840 has BLE but no Wi-Fi
    {},     // wifi_antenna_switch
    false,  // has_network — USB-CDC transport only

    // No dedicated UART: the openHop protocol runs on native USB-CDC Serial.
    -1,      // pin_protocol_uart_rx
    -1,      // pin_protocol_uart_tx
    921600,  // protocol_uart_baud

    -1,     // pin_gps_uart_rx
    -1,     // pin_gps_uart_tx
    9600,   // gps_uart_baud
    -1,     // pin_gps_enable
    true,   // gps_enable_active_high
    -1,     // pin_gps_reset
    false,  // gps_reset_active_high
    true,   // gps_send_casic_config

    // No Ethernet PHY or W5100S transport in the USB-only build.
    {false, BoardConfig::EthernetPhy::NONE, -1, -1, -1, -1, false, false,
     {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},

    {},  // pmu — no PMU chip on this board

    // Official MeshCore RAK4631 firmware drives SX126X_POWER_EN high before
    // radio init. Keep that fixed level for every RAK4631 transport variant.
    {{OPENHOP_RAK4631_SX126X_POWER_EN, true}, {-1, false}, {-1, false}, {-1, false}},
    1,
};
