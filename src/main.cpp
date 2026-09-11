#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include "SSD1306Wire.h"
#include "config.h"

// ----------------------------------------------------------------------------
// DBR-NET-006 PART 5: HELTEC V3 LPS8v2 DIRECT RANGE TESTER
// ----------------------------------------------------------------------------

// Heltec V3 SSD1306 OLED (0x3c address, SDA, SCL)
SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);

// RadioLib SX1262 Transceiver
Module radioModule(RADIO_NSS, RADIO_DIO1, RADIO_RST, RADIO_BUSY);
SX1262 radio = &radioModule;

// LoRaWAN Node Instance configured for IN865 region (per DBR §5.1)
LoRaWANNode node(&radio, &IN865);

// Walk-Test Metrics & State
uint32_t totalSentCount = 0;
uint32_t ackCount = 0;
int lastRssi = 0;
float lastSnr = 0.0f;
float lastVbat = 0.0f;
bool lastAckStatus = false;
int lastTxErrorCode = 0; // 0 = No error (or normal NO_RX_WINDOW), non-zero = RadioLib TX fault code
bool isJoined = false;
unsigned long lastTxTime = 0;

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------

float readBatteryVoltage() {
    // Heltec V3 uses GPIO 37 to enable ADC battery divider circuit
    pinMode(BATTERY_CTRL, OUTPUT);
    digitalWrite(BATTERY_CTRL, LOW); // Active LOW to enable battery divider
    delay(5);

    analogReadResolution(12);
    uint32_t raw = analogRead(BATTERY_ADC);
    
    digitalWrite(BATTERY_CTRL, HIGH); // Disable divider to save battery

    // 1:2 resistor divider formula on 3.3V ADC reference
    float voltage = ((float)raw / 4095.0f) * 3.3f * 2.0f;
    return voltage;
}

void drawSignalBars(int x, int y, int rssi, bool ackReceived) {
    // Draw 5-bar signal indicator based on RSSI thresholds
    int barHeights[5] = {4, 8, 12, 16, 20};
    int barWidth = 4;
    int gap = 2;

    int activeBars = 0;
    if (ackReceived) {
        if (rssi > -85)       activeBars = 5;
        else if (rssi > -95)  activeBars = 4;
        else if (rssi > -105) activeBars = 3;
        else if (rssi > -115) activeBars = 2;
        else if (rssi > -125) activeBars = 1;
    }

    for (int i = 0; i < 5; i++) {
        int bx = x + i * (barWidth + gap);
        int by = y + (20 - barHeights[i]);
        
        if (i < activeBars) {
            display.fillRect(bx, by, barWidth, barHeights[i]);
        } else {
            display.drawRect(bx, by, barWidth, barHeights[i]);
        }
    }
}

void updateOledDisplay(const String &statusLine, bool showMetrics = true) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    // Header
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "LPS8v2 Range Tester");
    display.drawString(0, 11, "--------------------------------");

    if (!showMetrics) {
        // Status / Join screen
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 26, statusLine);
        display.drawString(0, 42, "Region: IN865 (OTAA)");
    } else {
        // Main Walk-Test Display (per DBR §5.4)
        if (lastAckStatus) {
            // Large RSSI Readout
            display.setFont(ArialMT_Plain_16);
            display.drawString(0, 24, String(lastRssi) + " dBm");

            // Signal Strength 5-Bar Meter
            drawSignalBars(90, 24, lastRssi, true);
        } else if (lastTxErrorCode != 0) {
            // Hardware / Radio TX Error State
            display.setFont(ArialMT_Plain_16);
            display.drawString(0, 24, "TX ERR " + String(lastTxErrorCode));
            drawSignalBars(90, 24, -999, false);
        } else {
            // Normal NO ACK State (out of range / missed ACK)
            display.setFont(ArialMT_Plain_16);
            display.drawString(0, 24, "NO ACK");
            drawSignalBars(90, 24, -999, false);
        }

        // Metrics & Reach %
        display.setFont(ArialMT_Plain_10);
        float reachPct = (totalSentCount > 0) ? ((float)ackCount / (float)totalSentCount) * 100.0f : 0.0f;
        
        String metricsLine1 = "SNR: " + String(lastSnr, 1) + " dB | Vbat: " + String(lastVbat, 2) + "V";
        String metricsLine2 = "Reach: " + String((int)reachPct) + "% (n=" + String(totalSentCount) + ")";

        display.drawString(0, 43, metricsLine1);
        display.drawString(0, 54, metricsLine2);
    }

    display.display();
}

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==============================================");
    Serial.println("DBR-NET-006: 5F Urbanwoods LoRa Range Tester");
    Serial.println("Firmware: LPS8v2 Direct Tester (Part 5)");
    Serial.println("Region: IN865 | LoRaWAN OTAA | Confirmed Uplink");
    Serial.println("==============================================");
    Serial.println("CSV Header: millis,seq,rssi_dbm,snr_db,ack_received,vbat");

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

    // 3. Initialize OLED
    display.init();
    display.flipScreenVertically();
    updateOledDisplay("Initializing Hardware...", false);

    // 4. Initialize SPI Bus with Heltec V3 Pinout
    SPI.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);

    // 5. Initialize SX1262 Radio
    Serial.print("[RadioLib] Initializing SX1262... ");
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS!");
    } else {
        Serial.printf("FAILED, code %d\n", state);
        updateOledDisplay("Radio Init Fail: " + String(state), false);
        while (true) { delay(1000); }
    }

    // 6. Set Heltec V3 TCXO Voltage (1.6V required for 32MHz crystal per DBR §3.1)
    Serial.print("[RadioLib] Setting TCXO 1.6V... ");
    state = radio.setTCXO(1.6);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS!");
    } else {
        Serial.printf("FAILED, code %d\n", state);
    }

    // 7. LoRaWAN OTAA Setup (IN865 Region)
    Serial.println("[LoRaWAN] Configuring OTAA Keys for IN865...");
    updateOledDisplay("Joining ChirpStack...", false);

    state = node.beginOTAA(LORAWAN_JOIN_EUI, LORAWAN_DEV_EUI, LORAWAN_APP_KEY, LORAWAN_NWK_KEY);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRaWAN] OTAA Config Error: %d\n", state);
        updateOledDisplay("OTAA Config Err: " + String(state), false);
    }

    // Attempt Initial Join
    state = node.activateOTAA();
    if (state == RADIOLIB_ERR_NONE) {
        isJoined = true;
        Serial.println("[LoRaWAN] OTAA JOIN SUCCESSFUL!");
        updateOledDisplay("Joined Network!", false);
    } else {
        Serial.printf("[LoRaWAN] OTAA Join Pending (Code %d). Will retry in loop.\n", state);
        updateOledDisplay("Joining Network... (Pending)", false);
    }

    delay(1500);
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------
void loop() {
    unsigned long now = millis();

    // 1. Join Loop (if not yet joined)
    if (!isJoined) {
        if (now - lastTxTime >= 10000) { // Retry join every 10s
            lastTxTime = now;
            Serial.println("[LoRaWAN] Retrying OTAA Join (IN865)...");
            updateOledDisplay("Retrying OTAA Join...", false);

            int state = node.activateOTAA();
            if (state == RADIOLIB_ERR_NONE) {
                isJoined = true;
                Serial.println("[LoRaWAN] OTAA JOIN SUCCESSFUL!");
                updateOledDisplay("Joined ChirpStack!", false);
            } else {
                Serial.printf("[LoRaWAN] Join attempt failed, code %d\n", state);
                updateOledDisplay("Join Failed (Err " + String(state) + ")", false);
            }
        }
        return;
    }

    // 2. Periodic Confirmed Uplink Loop (Every 10 seconds per DBR §5.1)
    if (now - lastTxTime >= UPLINK_INTERVAL_MS) {
        lastTxTime = now;
        totalSentCount++;

        lastVbat = readBatteryVoltage();

        // 1-Byte Telemetry Payload: [SeqCounter & 0xFF]
        uint8_t payload[1];
        payload[0] = (uint8_t)(totalSentCount & 0xFF);

        Serial.printf("\n[Tx #%u] Sending Confirmed Uplink (IN865)... ", totalSentCount);

        // Downlink event struct
        LoRaWANEvent_t eventDown;

        // Perform CONFIRMED send & receive (isConfirmed = true)
        // RadioLib sendReceive return values:
        //   state > 0: rxWindow > 0 (1 or 2), DOWNLINK/ACK RECEIVED! eventDown is populated.
        //   state == 0 (RADIOLIB_ERR_NONE): Uplink sent OK, but NO DOWNLINK/ACK received in RX1 or RX2.
        //   state < 0 (state < RADIOLIB_ERR_NONE): Hardware/Network TX error code.
        int state = node.sendReceive(payload, sizeof(payload), LORAWAN_FPORT, true, nullptr, &eventDown);

        if (state > 0) {
            // ACK & Downlink Received Successfully (state = RX Window 1 or 2)!
            ackCount++;
            lastAckStatus = true;
            lastTxErrorCode = 0;
            lastRssi = (int)eventDown.power; // eventDown.power contains RSSI per DBR §5.2
            lastSnr = radio.getSNR();       // Read downlink SNR

            Serial.printf("ACK OK! (RX Window %d) | Downlink RSSI: %d dBm | SNR: %.1f dB | Vbat: %.2fV\n",
                          state, lastRssi, lastSnr, lastVbat);
        } else {
            // No ACK or Hardware TX Fault
            lastAckStatus = false;
            lastRssi = -999;
            lastSnr = 0.0f;

            if (state == RADIOLIB_ERR_NONE) {
                // Uplink sent, but NO ACK/Downlink received in RX1 or RX2 (rxWindow == 0)
                lastTxErrorCode = 0;
                Serial.printf("NO ACK Received! (RX Window 0) | Vbat: %.2fV\n", lastVbat);
            } else {
                // Hardware / Radio TX Error (state < 0)
                lastTxErrorCode = state;
                Serial.printf("TX Error (Code %d) | Vbat: %.2fV\n", state, lastVbat);
            }
        }

        // Single unified CSV Log over USB Serial
        Serial.printf("CSV,%lu,%u,%d,%.1f,%d,%.2f\n",
                      now, totalSentCount, lastRssi, lastSnr, lastAckStatus ? 1 : 0, lastVbat);

        // Update OLED display with cached metrics
        updateOledDisplay("", true);
    }
}
