#include "NetworkManager.h"

NetworkManager::NetworkManager() : lastReconnectAttempt(0) {
    strcpy(moduleName, "default-module"); // Valeur par défaut
    fullTopic[0] = '\0';
    lastConnectionTime = millis(); // Initialiser pour éviter un reboot immédiat
}

void NetworkManager::begin() {
    loadModuleName();
    connectWiFi();
    setupMDNS();
    configureMQTT();
    setupMqttCallbacks();
    
    if (WiFi.status() == WL_CONNECTED) {
        lastConnectionTime = millis();
        
        delay(500); // Stabilisation WiFi
        reconnectAttempts = 0;
        reconnect();
    }
}

void NetworkManager::loop() {
    manageWiFi();

    if (WiFi.status() != WL_CONNECTED) return;
    
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        unsigned long interval = calculateBackoff();
        
        if (now - lastReconnectAttempt > interval) {
            lastReconnectAttempt = now;
            reconnectAttempts++;
            reconnect();
        }
    } else {
        reconnectAttempts = 0;
        lastConnectionTime = millis(); // Reset watchdog si MQTT connecté
    }
}

void NetworkManager::manageWiFi() {
    unsigned long now = millis();
    
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
             lastConnectionTime = now; 
        }
        return;
    }

    if (now - lastWifiReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
        lastWifiReconnectAttempt = now;
        WiFi.disconnect(); 
        WiFi.reconnect();
    }
    
    if (now - lastConnectionTime > WATCHDOG_TIMEOUT) {
        Serial.println(F("💀 Connection lost for too long (Watchdog), rebooting..."));
        delay(1000);
        ESP.restart();
    }
}

void NetworkManager::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttMessageCallback = callback;
    if (strlen(fullTopic) > 0) {
        setupMqttCallbacks();
    }
}

void NetworkManager::publishCO2(int ppm) {
    if (!mqttClient.connected()) {
        return;
    }
    String topic = String(fullTopic) + "/co2";
    mqttClient.publish(topic.c_str(), 0, false, String(ppm).c_str());
}

void NetworkManager::publishVocIndex(int vocIndex) {
    if (!mqttClient.connected()) return;
    String topic = String(fullTopic) + "/voc";
    mqttClient.publish(topic.c_str(), 0, false, String(vocIndex).c_str());
}

void NetworkManager::publishValue(const char* suffix, float value) {
    if (!mqttClient.connected()) return;
    String topic = String(fullTopic) + suffix;
    mqttClient.publish(topic.c_str(), 0, false, String(value, 1).c_str());
}

bool NetworkManager::publishMessage(const char* suffix, const char* payload, bool retained) {
    if (!mqttClient.connected()) return false;
    String topic = String(fullTopic) + suffix;
    return mqttClient.publish(topic.c_str(), 0, retained, payload) > 0;
}

const char* NetworkManager::getTopicPrefix() { return fullTopic; }
long NetworkManager::getRSSI() { return WiFi.RSSI(); }
IPAddress NetworkManager::getIP() { return WiFi.localIP(); }
bool NetworkManager::isConnected() { return mqttClient.connected(); }
bool NetworkManager::isWifiConnected() { return WiFi.status() == WL_CONNECTED; }

void NetworkManager::loadModuleName() {
    preferences.begin("sensor_config", false);
    String savedName = preferences.getString("name", "");
    preferences.end();

    if (savedName.length() > 0) {
        savedName.toCharArray(moduleName, sizeof(moduleName));
    }
    
    snprintf(fullTopic, sizeof(fullTopic), "%s", moduleName);
}

void NetworkManager::saveModuleName(const char* name) {
    strcpy(moduleName, name);
    snprintf(fullTopic, sizeof(fullTopic), "%s", moduleName);
    
    preferences.begin("sensor_config", false);
    preferences.putString("name", moduleName);
    preferences.end();
}

void NetworkManager::connectWiFi() {
    #ifdef WIFI_SSID
        // Connexion directe avec credentials hardcodés
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int attempts = 0;
        // On attend un peu (10s max) mais on ne bloque pas indéfiniment
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        // Pas de reboot ici, on laisse le loop() gérer
        
        // Configurer NTP
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    #else
        // Portail WiFi avec configuration du nom du module
        WiFiManagerParameter customModuleName("module_name", "Nom du Module", moduleName, 32);
        
        WiFiManager wifiManager;
        wifiManager.addParameter(&customModuleName);
        wifiManager.setSaveConfigCallback([this]() { this->shouldSaveConfig = true; });
        wifiManager.setTimeout(180);
        
        if (!wifiManager.autoConnect("ESP32-Sensor-Config")) {
            Serial.println(F("WiFi connection failed (timeout), rebooting..."));
            delay(3000);
            ESP.restart(); // Pour le portail, le reboot est plus logique car on attend une interaction utilisateur
        }
        
        // Configurer NTP
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        
        // Sauvegarder le nom du module si modifié
        if (shouldSaveConfig) {
            saveModuleName(customModuleName.getValue());
        }
    #endif
}

void NetworkManager::setupMDNS() {
    MDNS.begin("esp32-client");
}

void NetworkManager::configureMQTT() {
    String serverStr = String(MQTT_SERVER_ADDR);
    
    if (serverStr.endsWith(".local")) {
        // Résolution mDNS
        String host = serverStr;
        host.replace(".local", "");
        mqttServerIP = MDNS.queryHost(host);
        
        if (mqttServerIP != IPAddress(0, 0, 0, 0)) {
            mqttUseIP = true;
            mqttClient.setServer(mqttServerIP, MQTT_PORT);
        } else {
            mqttUseIP = false;
            mqttClient.setServer(MQTT_SERVER, MQTT_PORT); // Use macro here if possible or MQTT_SERVER_ADDR
        }
    } else {
        // Essayer de parser comme IP directe
        if (mqttServerIP.fromString(MQTT_SERVER_ADDR)) {
            mqttUseIP = true;
            mqttClient.setServer(mqttServerIP, MQTT_PORT);
        } else {
            mqttUseIP = false;
            mqttClient.setServer(MQTT_SERVER_ADDR, MQTT_PORT);
        }
    }
}

unsigned long NetworkManager::calculateBackoff() {
    if (reconnectAttempts == 0) return 1000;
    
    unsigned int shiftAmount = (reconnectAttempts - 1 > 2) ? 2 : (reconnectAttempts - 1);
    unsigned long interval = 1000UL * (1UL << shiftAmount);
    return (interval > 5000UL) ? 5000UL : interval;
}

void NetworkManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        if (onMqttReconnectAttemptCallback) {
            onMqttReconnectAttemptCallback(reconnectAttempts);
        }
        return;
    }
    
    // Re-vérifier l'IP via mDNS si nécessaire (au cas où l'IP du serveur a changé)
    String serverStr = String(MQTT_SERVER_ADDR);
    if (serverStr.endsWith(".local")) {
        String host = serverStr;
        host.replace(".local", "");
        IPAddress newIP = MDNS.queryHost(host);
        if (newIP != IPAddress(0, 0, 0, 0)) {
            if (newIP != mqttServerIP) {
                mqttServerIP = newIP;
                mqttClient.setServer(mqttServerIP, MQTT_PORT);
            }
        }
    }
    
    Serial.println(F("Connexion to MQTT ..."));
    
    if (onMqttReconnectAttemptCallback) {
        onMqttReconnectAttemptCallback(reconnectAttempts);
    }

    String clientId = "ESP32-" + String((uint32_t)random(0xffff), HEX);
    mqttClient.setClientId(clientId.c_str());
    mqttClient.setKeepAlive(60);
    mqttClient.setCleanSession(true);
    
    mqttClient.connect();
}

void NetworkManager::setupMqttCallbacks() {
    if (callbacksInitialized) return;
    
    mqttClient.onConnect([this](bool sessionPresent) {
        reconnectAttempts = 0;
        Serial.println(F("Connexion success"));

        String configTopic = String(fullTopic) + "/sensors/config";
        mqttClient.subscribe(configTopic.c_str(), 1);

        String resetTopic = String(fullTopic) + "/sensors/reset";
        mqttClient.subscribe(resetTopic.c_str(), 1);

        if (onMqttConnectedCallback) {
            onMqttConnectedCallback();
        }
    });

    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
        if (onMqttDisconnectedCallback) {
            onMqttDisconnectedCallback((int)reason);
        }
    });

    mqttClient.onMessage(
        [this](char* topic, char* payload, AsyncMqttClientMessageProperties properties,
               size_t len, size_t index, size_t total) {
            if (payload == nullptr || len == 0 || !mqttMessageCallback) return;

            uint8_t* payloadCopy = (uint8_t*)malloc(len);
            if (payloadCopy) {
                memcpy(payloadCopy, payload, len);
                mqttMessageCallback(topic, payloadCopy, len);
                free(payloadCopy);
            }
        });
        
    callbacksInitialized = true;
}
