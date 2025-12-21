#include "MqttHandler.h"
#include <Preferences.h>

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
    } else if (topicStr.endsWith("/sensors/enable")) {
        handleEnableMessage(msg);
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
    
    // CO2
    if (!updateInterval(sensors, "mhz14a:co2", &_config.co2Interval)) {
        updateInterval(sensors, "co2", &_config.co2Interval);
    }

    // DHT22 (Temp/Hum independent)
    if (!updateInterval(sensors, "dht22:temperature", &_config.tempInterval)) {
        updateInterval(sensors, "temperature", &_config.tempInterval);
    }
    if (!updateInterval(sensors, "dht22:humidity", &_config.humInterval)) {
        updateInterval(sensors, "humidity", &_config.humInterval);
    }

    // SGP40
    if (!updateInterval(sensors, "sgp40:voc", &_config.vocInterval)) {
        updateInterval(sensors, "voc", &_config.vocInterval);
    }

    // BMP280 (Uses pressureInterval for loop timing)
    if (updateInterval(sensors, "bmp280:temperature", &_config.pressureInterval)) {}
    else if (updateInterval(sensors, "bmp280:pressure", &_config.pressureInterval)) {}
    else {
        updateInterval(sensors, "pressure", &_config.pressureInterval);
    }

    // SGP30
    if (!updateInterval(sensors, "sgp30:eco2", &_config.eco2Interval)) {
        updateInterval(sensors, "eco2", &_config.eco2Interval);
    }
    if (!updateInterval(sensors, "sgp30:tvoc", &_config.tvocInterval)) {
        updateInterval(sensors, "tvoc", &_config.tvocInterval);
    }

    // SC16-CO
    if (!updateInterval(sensors, "sc16co:co", &_config.coInterval)) {
        updateInterval(sensors, "co", &_config.coInterval);
    }
    
    // PM sensors (SPS30)
    if (updateInterval(sensors, "sps30:pm1", &_config.pmInterval)) {}
    else if (updateInterval(sensors, "sps30:pm25", &_config.pmInterval)) {}
    else if (updateInterval(sensors, "sps30:pm4", &_config.pmInterval)) {}
    else if (updateInterval(sensors, "sps30:pm10", &_config.pmInterval)) {}
    else {
        // Legacy PM keys
        if (updateInterval(sensors, "pm1", &_config.pmInterval)) {}
        else if (updateInterval(sensors, "pm25", &_config.pmInterval)) {}
        else if (updateInterval(sensors, "pm4", &_config.pmInterval)) {}
        else {
             updateInterval(sensors, "pm10", &_config.pmInterval);
        }
    }
    
    // SHT (Temp & Hum shared)
    if (updateInterval(sensors, "sht40:temperature", &_config.shtInterval)) {}
    else if (updateInterval(sensors, "sht40:humidity", &_config.shtInterval)) {}
    else if (updateInterval(sensors, "temp_sht", &_config.shtInterval)) {}
    else {
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

// ============================================================================
// Hardware Enable/Disable Handler
// ============================================================================

void MqttHandler::handleEnableMessage(char* msg) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
        if (_logger) {
            char errMsg[64];
            snprintf(errMsg, sizeof(errMsg), "Enable parse error: %s", error.f_str());
            _logger->error(errMsg);
        }
        return;
    }
    
    if (!doc.containsKey("hardware") || !doc.containsKey("enabled")) return;
    
    const char* hardware = doc["hardware"];
    bool enabled = doc["enabled"];
    
    // Map hardware key to config flag
    if (strcmp(hardware, "dht22") == 0) {
        _config.dht22Enabled = enabled;
    } else if (strcmp(hardware, "bmp280") == 0) {
        _config.bmp280Enabled = enabled;
    } else if (strcmp(hardware, "sgp40") == 0) {
        _config.sgp40Enabled = enabled;
    } else if (strcmp(hardware, "sgp30") == 0) {
        _config.sgp30Enabled = enabled;
    } else if (strcmp(hardware, "sps30") == 0) {
        _config.sps30Enabled = enabled;
    } else if (strcmp(hardware, "sht40") == 0) {
        _config.sht40Enabled = enabled;
    } else if (strcmp(hardware, "mhz14a") == 0) {
        _config.mhz14aEnabled = enabled;
    } else if (strcmp(hardware, "sc16co") == 0) {
        _config.sc16coEnabled = enabled;
    } else {
        if (_logger) {
            char logMsg[64];
            snprintf(logMsg, sizeof(logMsg), "Unknown hardware: %s", hardware);
            _logger->warn(logMsg);
        }
        return;
    }
    
    // Persist to flash
    saveEnabledState();
    
    if (_logger) {
        char logMsg[48];
        snprintf(logMsg, sizeof(logMsg), "%s %s", hardware, enabled ? "enabled" : "disabled");
        _logger->info(logMsg);
    }
}

void MqttHandler::saveEnabledState() {
    Preferences prefs;
    prefs.begin("hw_enabled", false);
    prefs.putBool("dht22", _config.dht22Enabled);
    prefs.putBool("bmp280", _config.bmp280Enabled);
    prefs.putBool("sgp40", _config.sgp40Enabled);
    prefs.putBool("sgp30", _config.sgp30Enabled);
    prefs.putBool("sps30", _config.sps30Enabled);
    prefs.putBool("sht40", _config.sht40Enabled);
    prefs.putBool("mhz14a", _config.mhz14aEnabled);
    prefs.putBool("sc16co", _config.sc16coEnabled);
    prefs.end();
}

void MqttHandler::loadEnabledState() {
    Preferences prefs;
    prefs.begin("hw_enabled", true);  // Read-only
    _config.dht22Enabled = prefs.getBool("dht22", true);
    _config.bmp280Enabled = prefs.getBool("bmp280", true);
    _config.sgp40Enabled = prefs.getBool("sgp40", true);
    _config.sgp30Enabled = prefs.getBool("sgp30", true);
    _config.sps30Enabled = prefs.getBool("sps30", true);
    _config.sht40Enabled = prefs.getBool("sht40", true);
    _config.mhz14aEnabled = prefs.getBool("mhz14a", true);
    _config.sc16coEnabled = prefs.getBool("sc16co", true);
    prefs.end();
}
