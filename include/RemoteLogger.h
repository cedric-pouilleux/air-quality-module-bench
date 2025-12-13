#ifndef REMOTE_LOGGER_H
#define REMOTE_LOGGER_H

#include <Arduino.h>
#include "NetworkManager.h"

#define MAX_BUFFERED_LOGS 50

struct BufferedLog {
    String level;
    String message;
    uint64_t timestamp;
};

class RemoteLogger {
public:
    RemoteLogger(NetworkManager& network, const String& moduleId);
    
    void info(const String& message);
    void success(const String& message);
    void warn(const String& message);
    void error(const String& message);
    void debug(const String& message);
    
    // Envoie tous les logs en attente une fois MQTT connecté
    void flushBufferedLogs();
    
private:
    NetworkManager& _network;
    String _moduleId;
    String _logTopic;
    
    BufferedLog _bufferedLogs[MAX_BUFFERED_LOGS];
    int _bufferedLogCount;
    
    void publishLog(const String& level, const String& message);
    void bufferLog(const String& level, const String& message);
};

#endif
