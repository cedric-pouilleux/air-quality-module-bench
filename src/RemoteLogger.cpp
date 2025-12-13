#include "RemoteLogger.h"
#include <ArduinoJson.h>
#include <sys/time.h>

// Helper pour obtenir le timestamp (NTP si dispo, sinon millis)
static uint64_t getCurrentTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Si l'heure est valide (> 2020, soit > 1600000000 sec)
    if (tv.tv_sec > 1600000000) {
        return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
    return millis();
}

RemoteLogger::RemoteLogger(NetworkManager& network, const String& moduleId)
    : _network(network), _moduleId(moduleId), _bufferedLogCount(0) {
    _logTopic = _moduleId + "/logs";
}

void RemoteLogger::info(const String& message) {
    publishLog("info", message);
}

void RemoteLogger::success(const String& message) {
    publishLog("success", message);
}

void RemoteLogger::warn(const String& message) {
    publishLog("warn", message);
}

void RemoteLogger::error(const String& message) {
    // Errors are still logged to Serial for immediate debugging
    Serial.print("[ERROR] ");
    Serial.println(message);
    publishLog("error", message);
}

void RemoteLogger::debug(const String& message) {
    publishLog("debug", message);
}

void RemoteLogger::publishLog(const String& level, const String& message) {
    if (!_network.isConnected()) {
        // Buffer le log si MQTT n'est pas connecté
        bufferLog(level, message);
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["level"] = level;
    doc["msg"] = message;
    doc["time"] = getCurrentTimestamp();
    
    String payload;
    serializeJson(doc, payload);
    
    _network.publishMessage("/logs", payload.c_str());
}

void RemoteLogger::bufferLog(const String& level, const String& message) {
    if (_bufferedLogCount >= MAX_BUFFERED_LOGS) {
        // Buffer plein, on supprime le plus ancien (FIFO)
        for (int i = 0; i < MAX_BUFFERED_LOGS - 1; i++) {
            _bufferedLogs[i] = _bufferedLogs[i + 1];
        }
        _bufferedLogCount = MAX_BUFFERED_LOGS - 1;
    }
    
    _bufferedLogs[_bufferedLogCount].level = level;
    _bufferedLogs[_bufferedLogCount].message = message;
    _bufferedLogs[_bufferedLogCount].timestamp = getCurrentTimestamp();
    _bufferedLogCount++;
}

void RemoteLogger::flushBufferedLogs() {
    if (!_network.isConnected() || _bufferedLogCount == 0) {
        return;
    }
    
    // Envoyer tous les logs en attente
    for (int i = 0; i < _bufferedLogCount; i++) {
        StaticJsonDocument<256> doc;
        doc["level"] = _bufferedLogs[i].level;
        doc["msg"] = _bufferedLogs[i].message;
        doc["time"] = _bufferedLogs[i].timestamp;
        
        String payload;
        serializeJson(doc, payload);
        
        _network.publishMessage("/logs", payload.c_str());
        
        // Petit délai pour éviter de surcharger MQTT
        delay(10);
    }
    
    _bufferedLogCount = 0;
}
