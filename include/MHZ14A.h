#ifndef MHZ14A_H
#define MHZ14A_H

#include <Arduino.h>

class MHZ14A {
private:
    HardwareSerial& serial;
    int lastPpm;
    
    byte calculateChecksum(byte* packet) {
        byte sum = 0;
        for (int i = 1; i < 8; i++) sum += packet[i];
        return 0xFF - sum + 1;
    }

    bool checkChecksum(byte* packet) {
        return (calculateChecksum(packet) == packet[8]);
    }

public:
    MHZ14A(HardwareSerial& s) : serial(s), lastPpm(-1) {}

    void begin(int rxPin, int txPin) {
        serial.begin(9600, SERIAL_8N1, rxPin, txPin);
    }

    void reset() {
        byte cmd[9] = {0xFF, 0x01, 0x87, 0, 0, 0, 0, 0, 0};
        cmd[8] = calculateChecksum(cmd);
        sendCommand(cmd);
        delay(2000);
    }

    void disableABC() {
        byte cmd[9] = {0xFF, 0x01, 0x79, 0x00, 0, 0, 0, 0, 0};
        cmd[8] = calculateChecksum(cmd);
        sendCommand(cmd);
        delay(100);
    }

    void enableABC() {
        // 0xA0 = Active la calibration automatique (ABC)
        byte cmd[9] = {0xFF, 0x01, 0x79, 0xA0, 0, 0, 0, 0, 0};
        cmd[8] = calculateChecksum(cmd);
        sendCommand(cmd);
        delay(100);
    }

    void sendCommand(byte* cmd) {
        while (serial.available()) serial.read();
        serial.write(cmd, 9);
        serial.flush();
    }

    int readPPM() {
        byte cmd[9] = {0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79};
        byte resp[9] = {0};

        sendCommand(cmd);
        delay(100);

        unsigned long start = millis();
        while (serial.available() < 9 && (millis() - start) < 200) {
            delay(1);
        }

        if (serial.available() >= 9) {
            serial.readBytes(resp, 9);
            
            // Validation basique
            if (resp[0] != 0xFF || resp[1] != 0x86) return -1;
            if (!checkChecksum(resp)) return -1;

            int ppm = resp[2] * 256 + resp[3];

            // Filtrage des valeurs aberrantes
            if (ppm <= 0 || ppm > 5000 || ppm == 5000) return -1;

            // Filtrage des sauts brusques (>1000ppm d'un coup)
            if (lastPpm > 0 && abs(ppm - lastPpm) > 1000) return -1;
            
            lastPpm = ppm;
            return ppm;
        }
        return -1;
    }
};

#endif

