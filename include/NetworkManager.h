#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>

/**
 * @brief Manages WiFi and MQTT connections for the ESP32.
 * 
 * This class handles:
 * - WiFi connection (direct or via captive portal)
 * - MQTT configuration and connection maintenance
 * - Module name configuration
 */
class NetworkManager {
private:
    // MQTT Configuration (from build flags or defaults)
    #ifdef MQTT_SERVER
        static constexpr const char* MQTT_SERVER_ADDR = MQTT_SERVER;
    #else
        static constexpr const char* MQTT_SERVER_ADDR = "growbrain.local";
    #endif
    static constexpr uint16_t MQTT_PORT = 1883;
    
    // Module configuration
    char moduleName[32];
    char fullTopic[64];
    
    // Configuraion helpers
    IPAddress mqttServerIP;
    bool mqttUseIP = false;

    // State
    AsyncMqttClient mqttClient;
    Preferences preferences;
    
    unsigned long lastReconnectAttempt;
    unsigned long lastWifiReconnectAttempt = 0;
    unsigned long lastConnectionTime = 0;
    unsigned int reconnectAttempts = 0;
    bool shouldSaveConfig = false;
    bool callbacksInitialized = false;
    
    // Constants
    static constexpr unsigned long WIFI_RECONNECT_INTERVAL = 10000; // 10s
    static constexpr unsigned long WATCHDOG_TIMEOUT = 600000;       // 10 min

    // Internal Callback
    void (*mqttMessageCallback)(char*, uint8_t*, unsigned int) = nullptr;

public:
    // Public Callbacks
    void (*onMqttConnectedCallback)() = nullptr;
    void (*onMqttReconnectAttemptCallback)(unsigned int attempt) = nullptr;
    void (*onMqttDisconnectedCallback)(int reason) = nullptr;

    NetworkManager();

    /**
     * @brief Initializes WiFi and MQTT.
     * Loads config, connects WiFi, sets up mDNS and MQTT.
     */
    void begin();

    /**
     * @brief Main loop to maintain connections. 
     * Handles WiFi reconnection and MQTT keep-alive/reconnection.
     */
    void loop();

    /**
     * @brief Manages WiFi connection state and Watchdog.
     */
    void manageWiFi();

    /**
     * @brief Sets the callback for incoming MQTT messages.
     */
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));

    // Publishing methods
    void publishCO2(int ppm);
    void publishVocIndex(int vocIndex);
    void publishValue(const char* suffix, float value);
    bool publishMessage(const char* suffix, const char* payload, bool retained = false);

    // Getters
    const char* getTopicPrefix();
    long getRSSI();
    IPAddress getIP();
    bool isConnected();
    bool isWifiConnected();

private:
    void loadModuleName();
    void saveModuleName(const char* name);
    void connectWiFi();
    void setupMDNS();
    void configureMQTT();
    unsigned long calculateBackoff();
    void reconnect();
    void setupMqttCallbacks();
};

#endif // NETWORK_MANAGER_H
