#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include "SSD1306Wire.h"
#include "config.h"

// ----------------------------------------------------------------------------
// GLOBAL OBJECTS
// ----------------------------------------------------------------------------
// Heltec V3 SSD1306 OLED (0x3c address, SDA, SCL)
SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);

// RadioLib SX1262 Driver
Module radioModule(RADIO_NSS, RADIO_DIO1, RADIO_RST, RADIO_BUSY);
SX1262 radio = &radioModule;

// LoRaWAN Node Instance (RadioLib)
LoRaWANNode node(&radio, &EU868); // Default region EU868 - user can change region in config

// Telemetry & Packet State
uint32_t packetCounter = 0;
unsigned long lastTxTime = 0;
bool isJoined = false;

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------
void updateDisplay(const String &status, const String &subtext1 = "", const String &subtext2 = "") {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    display.drawString(0, 0, "Heltec V3 Range Finder");
    display.drawString(0, 12, "--------------------------------");
    display.drawString(0, 24, "Status: " + status);
    
    if (subtext1.length() > 0) {
        display.drawString(0, 38, subtext1);
    }
    if (subtext2.length() > 0) {
        display.drawString(0, 50, subtext2);
    }
    
    display.display();
}

float readBatteryVoltage() {
    // Read ADC and convert to Voltage (Heltec V3 has resistor divider on GPIO 1)
    analogReadResolution(12);
    uint32_t raw = analogRead(BATTERY_ADC);
    float voltage = (float)raw / 4095.0f * 3.3f * 2.0f; // 1:2 voltage divider
    return voltage;
}

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n==============================================");
    Serial.println("Heltec LoRa32 v3 - Dragino LPS8N v2 Range Test");
    Serial.println("==============================================");

    // 1. Turn ON Vext Power (GPIO 36 LOW powers OLED & SX1262)
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);
    delay(100);

    // 2. Hardware Reset OLED
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);
    delay(50);

    // 3. Initialize Display
    display.init();
    display.flipScreenVertically();
    updateDisplay("Initializing...");

    // 4. Initialize SPI Bus with Heltec V3 Pins
    SPI.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);

    // 5. Initialize SX1262 Radio
    Serial.print("[RadioLib] Initializing SX1262... ");
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS!");
    } else {
        Serial.printf("FAILED, code %d\n", state);
        updateDisplay("Radio Init Fail", "Code: " + String(state));
        while (true) { delay(1000); }
    }

    // 6. Set Heltec V3 TCXO Voltage (1.6V required for Heltec V3 32MHz crystal!)
    Serial.print("[RadioLib] Setting TCXO 1.6V... ");
    state = radio.setTCXO(1.6);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS!");
    } else {
        Serial.printf("FAILED, code %d\n", state);
    }

    // 7. LoRaWAN OTAA Setup
    Serial.println("[LoRaWAN] Configuring Keys & OTAA...");
    updateDisplay("Joining Network...", "ChirpStack OTAA");

    // Begin LoRaWAN Node setup
    state = node.beginOTAA(LORAWAN_JOIN_EUI, LORAWAN_DEV_EUI, LORAWAN_APP_KEY, LORAWAN_NWK_KEY);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[LoRaWAN] Join Request Sent!");
        updateDisplay("Joining...", "Connecting to LPS8N");
    } else {
        Serial.printf("[LoRaWAN] OTAA Config Failed, code %d\n", state);
        updateDisplay("Join Config Error", "Code: " + String(state));
    }

    // Perform Join Attempt
    state = node.activateOTAA();
    if (state == RADIOLIB_ERR_NONE) {
        isJoined = true;
        Serial.println("[LoRaWAN] JOINED ChirpStack Network Successfully!");
        updateDisplay("Network Joined!", "ChirpStack Ready");
    } else {
        Serial.printf("[LoRaWAN] Join Failed, code %d. Will retry in loop.\n", state);
        updateDisplay("Join Pending...", "Retrying OTAA");
    }

    delay(2000);
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------
void loop() {
    unsigned long now = millis();

    // Check if network needs joining / re-joining
    if (!isJoined) {
        if (now - lastTxTime >= 10000) { // Retry join every 10s
            lastTxTime = now;
            Serial.println("[LoRaWAN] Retrying OTAA Join...");
            updateDisplay("Joining...", "Attempting OTAA");
            
            int state = node.activateOTAA();
            if (state == RADIOLIB_ERR_NONE) {
                isJoined = true;
                Serial.println("[LoRaWAN] JOIN SUCCESSFUL!");
                updateDisplay("Joined ChirpStack!", "Ready for testing");
            } else {
                Serial.printf("[LoRaWAN] Join attempt failed, code %d\n", state);
                updateDisplay("Join Failed", "Retrying in 10s");
            }
        }
        return;
    }

    // Periodic Uplink Transmission
    if (now - lastTxTime >= UPLINK_INTERVAL_MS) {
        lastTxTime = now;
        packetCounter++;

        float vbat = readBatteryVoltage();
        Serial.printf("\n[Uplink #%u] Preparing Packet | Vbat: %.2fV\n", packetCounter, vbat);

        // Construct 6-byte Payload: [Counter (4 bytes)][Vbat*100 (2 bytes)]
        uint8_t payload[6];
        payload[0] = (packetCounter >> 24) & 0xFF;
        payload[1] = (packetCounter >> 16) & 0xFF;
        payload[2] = (packetCounter >> 8) & 0xFF;
        payload[3] = packetCounter & 0xFF;

        uint16_t vbatInt = (uint16_t)(vbat * 100.0f);
        payload[4] = (vbatInt >> 8) & 0xFF;
        payload[5] = vbatInt & 0xFF;

        updateDisplay("Sending Pkt #" + String(packetCounter), "Payload: 6 Bytes", "Vbat: " + String(vbat, 2) + "V");

        // Transmit Uplink on FPort 1
        int state = node.sendReceive(payload, sizeof(payload), 1);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("[Uplink] Packet sent & ACK/Downlink received!");
            updateDisplay("Pkt #" + String(packetCounter) + " Sent OK!", "Gateway Received", "RSSI/SNR on ChirpStack");
        } else if (state == RADIOLIB_ERR_NO_RX_WINDOW) {
            Serial.println("[Uplink] Packet sent (unconfirmed).");
            updateDisplay("Pkt #" + String(packetCounter) + " Sent OK!", "TX Success", "Waiting next interval");
        } else {
            Serial.printf("[Uplink] Send failed, code %d\n", state);
            updateDisplay("Pkt #" + String(packetCounter) + " Fail", "Code: " + String(state));
        }
    }
}
