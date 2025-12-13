#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OtaManager {
public:
    void begin(const char* hostname) {
        // Configuration du nom réseau (ex: esp32-salon)
        ArduinoOTA.setHostname(hostname);

        // Optionnel : Mot de passe pour l'upload
        // ArduinoOTA.setPassword("admin");

        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";
                Serial.println("OTA Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nOTA End");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("OTA Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });

        ArduinoOTA.begin();
        Serial.println("OTA service started");
    }

    void loop() {
        ArduinoOTA.handle();
    }
};

#endif

