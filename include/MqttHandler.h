#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SensorReader.h"
#include "SensorData.h"
#include "RemoteLogger.h"

/**
 * @brief Handles incoming MQTT messages for configuration and commands.
 */
class MqttHandler {
public:
    MqttHandler(SensorReader& reader, SensorConfig& config);
    
    /**
     * @brief Set the Logger instance
     */
    void setLogger(RemoteLogger* logger);

    /**
     * @brief Process an incoming MQTT message
     * 
     * @param topic Topic string
     * @param payload Payload bytes
     * @param length Payload length
     */
    void handleMessage(char* topic, byte* payload, unsigned int length);

private:
    SensorReader& _reader;
    SensorConfig& _config;
    RemoteLogger* _logger = nullptr;

    void handleConfigMessage(char* msg);
    void handleResetMessage(char* msg);
};

#endif
