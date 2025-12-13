#include "MqttHandler.h"

MqttHandler::MqttHandler(SensorReader& reader, SensorConfig& config)
    : _reader(reader), _config(config) {
}

void MqttHandler::setLogger(RemoteLogger* logger) {
    _logger = logger;
}

void MqttHandler::handleMessage(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';
    
    String topicStr = String(topic);

    if (topicStr.endsWith("/sensors/config")) {
        handleConfigMessage(msg);
    } else if (topicStr.endsWith("/sensors/reset")) {
        handleResetMessage(msg);
    }
}

// Helper to reduce config handling duplication
bool MqttHandler::updateInterval(JsonObject& sensors, const char* key, unsigned long* configValue) {
    if (!sensors.containsKey(key)) return false;
    
    unsigned long interval = sensors[key]["interval"];
    if (interval >= 5) {
        *configValue = interval * 1000;
        if (_logger) {
            char logMsg[48];
            snprintf(logMsg, sizeof(logMsg), "%s interval: %lus", key, interval);
            _logger->info(logMsg);
        }
        return true;
    }
    return false;
}

void MqttHandler::handleConfigMessage(char* msg) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error) {
        if (_logger) {
            char errMsg[64];
            snprintf(errMsg, sizeof(errMsg), "Config parse error: %s", error.f_str());
            _logger->error(errMsg);
        }
        return;
    }

    if (!doc.containsKey("sensors")) return;
    
    JsonObject sensors = doc["sensors"];
    
    updateInterval(sensors, "co2", &_config.co2Interval);
    updateInterval(sensors, "temperature", &_config.tempInterval);
    updateInterval(sensors, "humidity", &_config.humInterval);
    updateInterval(sensors, "voc", &_config.vocInterval);
    updateInterval(sensors, "pressure", &_config.pressureInterval);
    updateInterval(sensors, "eco2", &_config.eco2Interval);
    updateInterval(sensors, "tvoc", &_config.tvocInterval);
    
    // SHT shares interval for temp and hum
    if (updateInterval(sensors, "temp_sht", &_config.shtInterval)) {
        // Already updated
    } else {
        updateInterval(sensors, "hum_sht", &_config.shtInterval);
    }
}

void MqttHandler::handleResetMessage(char* msg) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error || !doc.containsKey("sensor")) return;
    
    String sensor = doc["sensor"].as<String>();
    if (_logger) {
        char logMsg[64];
        snprintf(logMsg, sizeof(logMsg), "Reset command: %s", sensor.c_str());
        _logger->warn(logMsg);
    }
    
    if (sensor == "bmp" || sensor == "pressure" || sensor == "all") {
        _reader.resetBMP();
    }
    if (sensor == "sgp" || sensor == "voc" || sensor == "all") {
        _reader.resetSGP();
    }
    if (sensor == "dht" || sensor == "temp" || sensor == "humidity" || sensor == "all") {
        _reader.resetDHT();
    }
    if (sensor == "co2" || sensor == "all") {
        _reader.resetCO2();
    }
    if (sensor == "sht" || sensor == "temp_sht" || sensor == "hum_sht" || sensor == "all") {
        _reader.resetSHT();
    }
}
