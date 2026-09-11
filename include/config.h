#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// CHIRPSTACK / LORAWAN CREDENTIALS (OTAA)
// Replace these with your device credentials from ChirpStack
// ============================================================================
// JoinEUI / AppEUI (8 bytes, MSB format)
static const uint64_t LORAWAN_JOIN_EUI = 0x0000000000000000ULL;

// DevEUI (8 bytes, MSB format) - Update with your ChirpStack DevEUI
static const uint64_t LORAWAN_DEV_EUI  = 0x70B3D57ED0000001ULL;

// AppKey (16 bytes) - Update with your ChirpStack AppKey
static const uint8_t LORAWAN_APP_KEY[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

// NwkKey (16 bytes, for LoRaWAN 1.1 - default same as AppKey for 1.0.x)
static const uint8_t LORAWAN_NWK_KEY[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

// ============================================================================
// HELTEC WIFI LORA 32 V3 HARDWARE PINOUT
// ============================================================================
#define VEXT_PIN      36   // Vext power control (Active LOW to enable power)

// SX1262 LoRa Transceiver Pins
#define RADIO_NSS     8
#define RADIO_SCK     9
#define RADIO_MOSI    10
#define RADIO_MISO    11
#define RADIO_RST     12
#define RADIO_BUSY    13
#define RADIO_DIO1    14

// OLED Display Pins (SSD1306)
#define OLED_SDA      17
#define OLED_SCL      18
#define OLED_RST      21

// Battery Voltage ADC Pin
#define BATTERY_ADC   1

// ============================================================================
// LORAWAN TRANSMISSION SETTINGS
// ============================================================================
#define UPLINK_INTERVAL_MS  15000   // Send uplink every 15 seconds
#define TX_POWER_DBM        14      // Transmit power in dBm

#endif // CONFIG_H
