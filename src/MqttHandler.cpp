#include "MqttHandler.h"

MqttHandler::MqttHandler(SensorReader& reader, SensorConfig& config)
    : _reader(reader), _config(config) {
}

void MqttHandler::setLogger(RemoteLogger* logger) {
    _logger = logger;
}

void MqttHandler::handleMessage(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
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

void MqttHandler::handleConfigMessage(char* msg) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error) {
        if (_logger) {
            char configErrMsg[80];
            snprintf(configErrMsg, sizeof(configErrMsg), "Config parse error: %s", error.f_str());
            _logger->error(configErrMsg);
        }
        return;
    }

    if (doc.containsKey("sensors")) {
        JsonObject sensors = doc["sensors"];
        
        if (sensors.containsKey("co2")) {
            unsigned long interval = sensors["co2"]["interval"];
            if (interval >= 5) {
                _config.co2Interval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "CO2 interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }
        
        if (sensors.containsKey("temperature")) {
            unsigned long interval = sensors["temperature"]["interval"];
            if (interval >= 5) {
                _config.tempInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "Temperature interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }
        
        if (sensors.containsKey("humidity")) {
            unsigned long interval = sensors["humidity"]["interval"];
            if (interval >= 5) {
                _config.humInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "Humidity interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("voc")) {
            unsigned long interval = sensors["voc"]["interval"];
            if (interval >= 5) {
                _config.vocInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "VOC interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("pressure")) {
            unsigned long interval = sensors["pressure"]["interval"];
            if (interval >= 5) {
                _config.pressureInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "Pressure interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("eco2")) {
            unsigned long interval = sensors["eco2"]["interval"];
            if (interval >= 5) {
                _config.eco2Interval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "eCO2 interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("tvoc")) {
            unsigned long interval = sensors["tvoc"]["interval"];
            if (interval >= 5) {
                _config.tvocInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "TVOC interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("temp_sht")) {
            unsigned long interval = sensors["temp_sht"]["interval"];
            if (interval >= 5) {
                _config.shtInterval = interval * 1000;
                // Also update humidity if shared? For now separate config keys but shared interval variable
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "SHT Temp interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }

        if (sensors.containsKey("hum_sht")) {
            unsigned long interval = sensors["hum_sht"]["interval"];
            if (interval >= 5) {
                _config.shtInterval = interval * 1000;
                if (_logger) {
                    char logMsg[64];
                    snprintf(logMsg, sizeof(logMsg), "SHT Hum interval: %lus", interval);
                    _logger->info(logMsg);
                }
            }
        }
    }
}

void MqttHandler::handleResetMessage(char* msg) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (!error && doc.containsKey("sensor")) {
        String sensor = doc["sensor"].as<String>();
        if (_logger) {
            char logMsg[64];
            snprintf(logMsg, sizeof(logMsg), "🔄 Received reset command for sensor: %s", sensor.c_str());
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
}
