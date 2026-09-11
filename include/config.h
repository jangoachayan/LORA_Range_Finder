#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DBR-NET-006 PART 5: CHIRPSTACK LORAWAN IN865 CREDENTIALS (OTAA)
// ============================================================================
// JoinEUI / AppEUI (8 bytes, MSB format)
static const uint64_t LORAWAN_JOIN_EUI = 0x0000000000000000ULL;

// DevEUI (8 bytes, MSB format) - Registered in ChirpStack
static const uint64_t LORAWAN_DEV_EUI  = 0x70B3D57ED0000001ULL;

// AppKey (16 bytes) - Registered in ChirpStack
static const uint8_t LORAWAN_APP_KEY[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

// NwkKey (16 bytes, for LoRaWAN 1.0.3 / 1.1)
static const uint8_t LORAWAN_NWK_KEY[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

// ============================================================================
// HELTEC WIFI LORA 32 V3 HARDWARE PINOUT (Verified per DBR Part 3.1)
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

// Battery Voltage ADC & Control Pins
#define BATTERY_ADC     1
#define BATTERY_CTRL   37

// ============================================================================
// DBR PART 5 TIMING & LORAWAN PARAMETERS
// ============================================================================
#define UPLINK_INTERVAL_MS  10000   // 10-second confirmed uplink cycle per DBR §5.1
#define LORAWAN_FPORT       1       // FPort for telemetry
#define TX_POWER_DBM        14      // Transmit power in dBm

#endif // CONFIG_H
