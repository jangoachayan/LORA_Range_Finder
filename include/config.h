#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DBR-NET-006 PART 5: CHIRPSTACK LORAWAN IN865 CREDENTIALS (OTAA)
// ============================================================================
// JoinEUI / AppEUI (8 bytes, MSB format)
static const uint64_t LORAWAN_JOIN_EUI = 0x992C012724ED92E2ULL;

// DevEUI (8 bytes, MSB format) - Registered in ChirpStack
static const uint64_t LORAWAN_DEV_EUI  = 0xB83D5A4C3A5380EDULL;

// AppKey (16 bytes) - Registered in ChirpStack
static const uint8_t LORAWAN_APP_KEY[] = {
    0x9A, 0xB3, 0x95, 0x95, 0x98, 0xF0, 0xE7, 0x3F,
    0x4D, 0x0C, 0x35, 0xD2, 0xDA, 0x8F, 0xA1, 0x99
};

// NwkKey (16 bytes, for LoRaWAN 1.0.3 / 1.1)
static const uint8_t LORAWAN_NWK_KEY[] = {
    0x9A, 0xB3, 0x95, 0x95, 0x98, 0xF0, 0xE7, 0x3F,
    0x4D, 0x0C, 0x35, 0xD2, 0xDA, 0x8F, 0xA1, 0x99
};

// ============================================================================
// HELTEC WIFI LORA 32 V3 HARDWARE PINOUT (Verified per DBR Part 3.1)
// ============================================================================
#define VEXT_PIN      36   // Vext power control (Active LOW to enable power)

// PRG / User Button Pin (Heltec V3 PRG button, Active LOW with internal pullup)
#define BUTTON_PIN    0

// Onboard White LED Pin (Heltec V3 White LED, Active HIGH)
#define LED_PIN       35

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
#define DEBOUNCE_DELAY_MS   200     // 200ms button debounce threshold
#define LORAWAN_FPORT       1       // FPort for telemetry
#define TX_POWER_DBM        22      // Transmit power in dBm (22 dBm max hardware capacity for Heltec V3 SX1262)

#endif // CONFIG_H
