// =============================================================
// boards/lilygo_tbeam_s3_supreme.h — LilyGO T-Beam-S3 Supreme
// ESP32-S3 + bare SX1262 (868 MHz) + L76K GNSS + 1.3" SH1106 OLED.
//
// Pin map + AXP2101 PMU rail assignments taken from LilyGO's own upstream
// variant (meshtastic/firmware variants/esp32s3/tbeam-s3-core and the
// LILYGO_TBEAM_S3_CORE branch of src/Power.cpp — same hardware as this
// board). Unlike every other board in this repo, LoRa/GNSS/OLED power
// isn't gated by plain GPIOs: it runs through an AXP2101 PMU chip on its
// own I2C bus (see PmuManager / BoardConfig.pmu in board_config.h).
// =============================================================
#pragma once

inline const BoardConfig BOARD = {
    .name        = "LilyGO T-Beam-S3 Supreme",
    .fw_suffix   = "lilygo-tbeam-s3-supreme",
    .mdns_prefix = "lilygo-tbeam-s3-supreme",

    // SX1262 control + SPI bus (remapped vs. the ESP32-S3 default)
    .pin_lora_nss  = 10,
    .pin_lora_rst  = 5,
    .pin_lora_busy = 4,
    .pin_lora_dio1 = 1,
    .pin_lora_sck  = 12,
    .pin_lora_miso = 13,
    .pin_lora_mosi = 11,

    .rf_switch = {
        .en_pin            = -1,   // bare SX1262 — RF switch is internal
        .en_low_hold_ms    = 0,
        .rx_pin            = -1,
        .tx_pin            = -1,
        .dio2_as_rf_switch = true,
    },

    // Onboard 1.3" SH1106 OLED. Shares oled_display's SH1106 driver with
    // Station G2 (see BOARD_STATION_G2 ifdef in oled_display.h/.cpp).
    // No dedicated reset line — the whole rail is power-cycled via the
    // PMU's ALDO1 instead.
    .pin_i2c_sda      = 17,
    .pin_i2c_scl      = 18,
    .pin_i2c_oled_rst = -1,
    .pin_vext_enable_low = -1,

    .pin_user_button        = 0,   // BOOT/user button on GPIO0
    .user_button_active_low = true,

    .max_tx_power_dbm = 22,        // bare SX1262 chip ceiling, no PA/FEM

    .use_dio3_tcxo = true,
    .tcxo_voltage  = 1.8f,
    .sx126x_current_limit_ma = 140,
    .sx126x_rx_boosted_gain = true,

    .has_lora_radio = true,
    .has_wifi       = true,
    .has_network    = true,
    .pin_protocol_uart_rx = -1,
    .pin_protocol_uart_tx = -1,
    .protocol_uart_baud   = 921600,

    // L76K GNSS on its own UART. NMEA-only module — no CASIC config
    // sentences. Power comes from the PMU's ALDO4, not a GPIO enable.
    .pin_gps_uart_rx = 9,
    .pin_gps_uart_tx = 8,
    .gps_uart_baud   = 9600,
    .pin_gps_enable  = -1,
    .pin_gps_reset   = -1,
    .gps_send_casic_config = false,

    .ethernet = { .enabled = false },

    // AXP2101 PMU on its own I2C bus (separate from the SDA=17/SCL=18
    // OLED/sensor bus above). All four rails are OFF at reset and must
    // be enabled here before OLED/radio/GNSS init touch them.
    .pmu = {
        .enabled = true,
        .i2c_sda = 42,
        .i2c_scl = 41,
        .aldo1   = true,   // sensor/OLED rail
        .aldo2   = true,   // I2C bus enable
        .aldo3   = true,   // LoRa radio rail
        .aldo4   = true,   // GNSS rail
    },
};
