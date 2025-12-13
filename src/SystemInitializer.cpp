#include "SystemInitializer.h"
#include <DHT_U.h>
#include <WiFi.h>

void SystemInitializer::initHardware(HardwareSerial& co2Serial, 
                                     DHT_Unified& dht, NetworkManager& network, OtaManager& ota) {
    Serial.begin(115200);
    Serial.println("=== ESP32 CO2 + DHT Sensor ===");
    
    co2Serial.begin(9600, SERIAL_8N1, 25, 26); // MHZ_RX_PIN (25), MHZ_TX_PIN (26) - Moved from 16/17 (PSRAM conflict)
    dht.begin();
    
    Serial.println("Connecting to WiFi...");
    network.begin();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("MAC: %s | RSSI: %d dBm\n", WiFi.macAddress().c_str(), WiFi.RSSI());
    }

    ota.begin(generateOtaHostname().c_str());
}

void SystemInitializer::configureSensor() {
    Serial.println("Initializing sensors...");
    delay(500);
}

void SystemInitializer::runWarmupSequence() {
    Serial.println("Warmup sequence...");
    delay(2000);
}

String SystemInitializer::generateOtaHostname() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return "esp32-" + mac.substring(6);
}

