/**
 * @file main.cpp
 * @brief ESP32 Air Quality Monitor - Refactored with iot-mesurable
 *
 * Uses the IotMesurable library for MQTT, WiFi, status publishing,
 * and enable/disable features. SensorReader handles hardware communication.
 */

#include <Arduino.h>
#include <Wire.h>
#include <DHT_U.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <IotMesurable.h>
#include "SensorReader.h"
#include "secrets.h"

// Priority logic: if a broker is defined via build flags (-D MQTT_HUB_IP), use it.
// This avoids conflicts with MQTT_SERVER defined in secrets.h
#ifdef MQTT_HUB_IP
  #define REAL_MQTT_SERVER MQTT_HUB_IP
#else
  #define REAL_MQTT_SERVER MQTT_SERVER
#endif

// ============================================================================
// Hardware Configuration
// ============================================================================

#define DHT_PIN 4
#define DHT_TYPE DHT22
#define CO_RX_PIN 14  // SC16-CO on GPIO 14 (RX)
#define CO_TX_PIN 12  // SC16-CO on GPIO 12 (TX)
#define MODULE_ID "module-esp32-1"

// ============================================================================
// Global Objects
// ============================================================================

// I2C buses
TwoWire wireSGP = TwoWire(1);

// Serial connections
HardwareSerial co2Serial(2);   // UART2 for MH-Z14A
HardwareSerial sps30Serial(1); // UART1 for SPS30
SoftwareSerial coSerial(CO_RX_PIN, CO_TX_PIN);

// DHT sensor
DHT_Unified dht(DHT_PIN, DHT_TYPE);

// Sensor reader (low-level hardware access)
SensorReader sensors(co2Serial, sps30Serial, dht, wireSGP, coSerial);

// IotMesurable - handles MQTT, WiFi, status, enable/disable
IotMesurable brain(MODULE_ID);

// ============================================================================
// Timing
// ============================================================================

struct SensorTiming {
    unsigned long lastRead = 0;
    unsigned long interval = 60000; // 60s default
};

SensorTiming timingMHZ14A;
SensorTiming timingDHT22;
SensorTiming timingSGP40;
SensorTiming timingSGP30;
SensorTiming timingSPS30;
SensorTiming timingBMP280;
SensorTiming timingSHT40;
SensorTiming timingSC16CO;

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Air Quality Monitor (iot-mesurable) ===\n");
    
    // Initialize I2C
    Wire.begin(21, 22);
    wireSGP.begin(32, 33); // SGP bus on 32, 33
    
    // Initialize serial ports
    co2Serial.begin(9600, SERIAL_8N1, 25, 26);  // MH-Z14A: RX=GPIO 25, TX=GPIO 26
    sps30Serial.begin(115200, SERIAL_8N1, 13, 27); // SPS30 on 13, 27
    coSerial.begin(9600);
    
    // Initialize brain (WiFi + MQTT)
    Serial.print("Connecting WiFi/MQTT...");
    brain.setBroker(REAL_MQTT_SERVER, 1883);
    if (!brain.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println(" FAILED!");
    } else {
        Serial.println(" OK");
    }
    
    // Register all hardware and sensors
    brain.setModuleType("air-quality-bench");
    brain.registerHardware("mhz14a", "MH-Z14A CO2 Sensor");
    brain.addSensor("mhz14a", "co2");
    
    brain.registerHardware("dht22", "DHT22 Temp/Humidity");
    brain.addSensor("dht22", "temperature");
    brain.addSensor("dht22", "humidity");
    
    brain.registerHardware("sgp40", "SGP40 VOC Sensor");
    brain.addSensor("sgp40", "voc");
    
    brain.registerHardware("sgp30", "SGP30 eCO2/TVOC");
    brain.addSensor("sgp30", "eco2");
    brain.addSensor("sgp30", "tvoc");
    
    brain.registerHardware("sps30", "SPS30 Particulate");
    brain.addSensor("sps30", "pm1");
    brain.addSensor("sps30", "pm25");
    brain.addSensor("sps30", "pm4");
    brain.addSensor("sps30", "pm10");
    
    brain.registerHardware("bmp280", "BMP280 Pressure");
    brain.addSensor("bmp280", "pressure");
    brain.addSensor("bmp280", "temperature");
    
    brain.registerHardware("sht40", "SHT40 Temp/Humidity");
    brain.addSensor("sht40", "temperature");
    brain.addSensor("sht40", "humidity");
    
    brain.registerHardware("sc16co", "SC16-CO Carbon Monoxide");
    brain.addSensor("sc16co", "co");
    
    // Callbacks for config/enable changes
    brain.onConfigChange([](const char* hw, int intervalMs) {
        Serial.printf("[Config] %s interval: %d ms\n", hw, intervalMs);
        if (strcmp(hw, "mhz14a") == 0) timingMHZ14A.interval = intervalMs;
        else if (strcmp(hw, "dht22") == 0) timingDHT22.interval = intervalMs;
        else if (strcmp(hw, "sgp40") == 0) timingSGP40.interval = intervalMs;
        else if (strcmp(hw, "sgp30") == 0) timingSGP30.interval = intervalMs;
        else if (strcmp(hw, "sps30") == 0) timingSPS30.interval = intervalMs;
        else if (strcmp(hw, "bmp280") == 0) timingBMP280.interval = intervalMs;
        else if (strcmp(hw, "sht40") == 0) timingSHT40.interval = intervalMs;
        else if (strcmp(hw, "sc16co") == 0) timingSC16CO.interval = intervalMs;
    });
    
    brain.onEnableChange([](const char* hw, bool enabled) {
        Serial.printf("[Enable] %s: %s\n", hw, enabled ? "ON" : "OFF");
    });

    brain.onConnect([](bool connected) {
        Serial.printf("[MQTT] %s\n", connected ? "Connected" : "Disconnected");
    });
    
    brain.onResetChange([](const char* hw) {
        Serial.printf("[Reset] Request for: %s\n", hw);
        brain.log("info", "Reset request received");
        
        bool success = false;
        
        if (strcmp(hw, "mhz14a") == 0) {
            // MH-Z14A doesn't have a specific init, but we can try to re-setup serial or similar if needed.
            // For now, re-init isn't explicitly exposed in SensorReader for CO2, 
            // but we can add resetCO2 if we want to be thorough.
            // Let's rely on what's available. 
            // SensorReader has resetCO2() (void).
            sensors.resetCO2();
            success = true;
        }
        else if (strcmp(hw, "dht22") == 0) {
           sensors.resetDHT();
           success = true;
        }
        else if (strcmp(hw, "sgp40") == 0) {
            success = sensors.initSGP();
        }
        else if (strcmp(hw, "sgp30") == 0) {
            success = sensors.initSGP30();
        }
        else if (strcmp(hw, "sps30") == 0) {
            success = sensors.initSPS30();
        }
        else if (strcmp(hw, "bmp280") == 0) {
            success = sensors.initBMP();
        }
        else if (strcmp(hw, "sht40") == 0) {
            success = sensors.initSHT();
        }
        else if (strcmp(hw, "sc16co") == 0) {
            success = sensors.initCO();
        }
        else {
             char msg[64];
             snprintf(msg, sizeof(msg), "Unknown hardware: %s", hw);
             brain.log("warn", msg);
             return;
        }
        
        if (success) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Hardware reset: %s", hw);
            brain.log("success", msg);
        } else {
             char msg[64];
             snprintf(msg, sizeof(msg), "Reset failed: %s", hw);
            brain.log("error", msg);
        }
    });
    
    // Initialize sensors
    Serial.println("Initializing sensors...");
    dht.begin();
    if (sensors.initBMP()) Serial.println(" - BMP280 OK");
    if (sensors.initSGP()) Serial.println(" - SGP40 OK");
    if (sensors.initSGP30()) Serial.println(" - SGP30 OK");
    if (sensors.initSPS30()) Serial.println(" - SPS30 OK");
    if (sensors.initSHT()) Serial.println(" - SHT40 OK");
    if (sensors.initCO()) Serial.println(" - SC16-CO OK");
    
    brain.log("info", "Module booted and connected");
    Serial.println("Setup complete!");
}

// ============================================================================
// Sensor Reading Helpers
// ============================================================================

bool shouldRead(SensorTiming& timing) {
    unsigned long now = millis();
    if (now - timing.lastRead >= timing.interval) {
        timing.lastRead = now;
        return true;
    }
    return false;
}

// ============================================================================
// Logging Helper
// ============================================================================

void logError(const char* msg) {
    // Log to Serial
    Serial.printf("[ERROR] %s\n", msg);
    
    // Log to MQTT via brain
    brain.log("error", msg);
}

// ============================================================================
// Loop
// ============================================================================

void loop() {
    brain.loop();
    
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 10000) {
        lastDebug = millis();
        Serial.printf("[Debug] IP: %s, MQTT: %s\n", 
            WiFi.localIP().toString().c_str(),
            brain.isConnected() ? "Yes" : "No");
    }

    // MH-Z14A (CO2)
    if (brain.isHardwareEnabled("mhz14a") && shouldRead(timingMHZ14A)) {
        int co2 = sensors.readCO2();
        Serial.printf("[MHZ14A] CO2: %d\n", co2);
        if (co2 > 0) {
            brain.publish("mhz14a", "co2", co2);
        } else {
            Serial.println("[MHZ14A] Read failed!");
            brain.publish("mhz14a", "co2", NAN);
        }
    }
    
    // DHT22 (Temp/Humidity)
    if (brain.isHardwareEnabled("dht22") && shouldRead(timingDHT22)) {
        DhtReading reading = sensors.readDhtSensors();
        Serial.printf("[DHT22] T: %.1f, H: %.1f\n", reading.temperature, reading.humidity);
        if (reading.valid) {
            brain.publish("dht22", "temperature", reading.temperature);
            brain.publish("dht22", "humidity", reading.humidity);
        } else {
            Serial.println("[DHT22] Read failed!");
            brain.publish("dht22", "temperature", NAN);
            brain.publish("dht22", "humidity", NAN);
        }
    }
    
    // SGP40 (VOC)
    if (brain.isHardwareEnabled("sgp40") && shouldRead(timingSGP40)) {
        int voc = sensors.readVocIndex();
        Serial.printf("[SGP40] VOC: %d\n", voc);
        if (voc >= 0) {
            brain.publish("sgp40", "voc", voc);
        } else {
            Serial.println("[SGP40] Read failed!");
            brain.publish("sgp40", "voc", NAN);
        }
    }
    
    // SGP30 (eCO2/TVOC)
    if (brain.isHardwareEnabled("sgp30") && shouldRead(timingSGP30)) {
        int eco2, tvoc;
        if (sensors.readSGP30(eco2, tvoc)) {
            Serial.printf("[SGP30] eCO2: %d, TVOC: %d\n", eco2, tvoc);
            brain.publish("sgp30", "eco2", eco2);
            brain.publish("sgp30", "tvoc", tvoc);
        } else {
            // Read failed or invalid (0) -> Report as missing
            Serial.println("[SGP30] Read failed!");
            brain.publish("sgp30", "eco2", NAN);
            brain.publish("sgp30", "tvoc", NAN);
        }
    }
    
    // SPS30 (PM)
    if (brain.isHardwareEnabled("sps30") && shouldRead(timingSPS30)) {
        float pm1, pm25, pm4, pm10;
        if (sensors.readSPS30(pm1, pm25, pm4, pm10)) {
            Serial.printf("[SPS30] PM2.5: %.1f\n", pm25);
            brain.publish("sps30", "pm1", pm1);
            brain.publish("sps30", "pm25", pm25);
            brain.publish("sps30", "pm4", pm4);
            brain.publish("sps30", "pm10", pm10);
        } else {
            Serial.println("[SPS30] Read failed!");
            brain.publish("sps30", "pm1", NAN);
            brain.publish("sps30", "pm25", NAN);
            brain.publish("sps30", "pm4", NAN);
            brain.publish("sps30", "pm10", NAN);
        }
    }
    
    // BMP280 (Pressure/Temp)
    if (brain.isHardwareEnabled("bmp280") && shouldRead(timingBMP280)) {
        float pressure = sensors.readPressure();
        float temp = sensors.readBMPTemperature();
        Serial.printf("[BMP280] P: %.1f, T: %.1f\n", pressure, temp);
        
        bool success = false;
        if (!isnan(pressure)) {
            brain.publish("bmp280", "pressure", pressure);
            success = true;
        } else {
             brain.publish("bmp280", "pressure", NAN);
        }
        
        if (!isnan(temp)) {
            brain.publish("bmp280", "temperature", temp);
            success = true;
        } else {
            brain.publish("bmp280", "temperature", NAN);
        }
        
        if (!success) Serial.println("[BMP280] Read failed!");
    }
    
    // SHT40 (Temp/Humidity)
    if (brain.isHardwareEnabled("sht40") && shouldRead(timingSHT40)) {
        float temp, hum;
        if (sensors.readSHT(temp, hum)) {
            Serial.printf("[SHT40] T: %.1f, H: %.1f\n", temp, hum);
            brain.publish("sht40", "temperature", temp);
            brain.publish("sht40", "humidity", hum);
        } else {
            Serial.println("[SHT40] Read failed!");
            brain.publish("sht40", "temperature", NAN);
            brain.publish("sht40", "humidity", NAN);
        }
    }
    
    // SC16-CO (Carbon Monoxide)
    if (brain.isHardwareEnabled("sc16co") && shouldRead(timingSC16CO)) {
        int co = sensors.readCO();
        Serial.printf("[SC16CO] CO: %d\n", co);
        if (co >= 0) {
            brain.publish("sc16co", "co", co);
        } else {
            char msg[128];
            // Format raw hex for MQTT string
            // Assuming _coBuffer is available only in SensorReader, we can't easily access it here.
            // Let's modify readCO to return a struct or handle logging inside SensorReader,
            // OR just return the error code and rely on the user checking the ACTIVITY LOGS detailed message if implemented.
            // For now, let's trust the error code is enough to say "Bad Protocol".
            // Actually, we can pass the error to the log.
            
            snprintf(msg, sizeof(msg), "SC16CO Err: %d", co);
            brain.log("error", msg);
            
            Serial.printf("[SC16CO] Read failed! Error code: %d\n", co);
             brain.publish("sc16co", "co", NAN);
        }
    }
}
