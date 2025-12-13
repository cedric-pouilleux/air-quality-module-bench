#include "SensorReader.h"
#include <Wire.h>
#include "RemoteLogger.h"

const uint8_t SensorReader::CO2_READ_CMD[9] = { 0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79 };

SensorReader::SensorReader(HardwareSerial& co2Serial, HardwareSerial& sps30Serial, DHT_Unified& dht, TwoWire& wireSGP) 
    : co2Serial(co2Serial), sps30Serial(sps30Serial), dht(dht), _wireSGP(wireSGP), sht(&wireSGP) {
}

void SensorReader::setLogger(RemoteLogger* logger) {
    _logger = logger;
}

bool SensorReader::initBMP(int maxAttempts, int delayBetweenMs) {
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        if (bmp.begin(0x76)) {
            if (attempt > 1) {
                String msg = "BMP280 initialized after " + String(attempt) + " attempts";
                Serial.println(msg);
                if (_logger) _logger->warn(msg);
            } else {
                String msg = "BMP280 initialized successfully";
                Serial.println(msg);
                if (_logger) _logger->info(msg);
            }
            return true;
        }
        if (attempt < maxAttempts) {
            delay(delayBetweenMs);
        }
    }
    String msg = "Could not find a valid BMP280 sensor after retries, check wiring!";
    Serial.println(msg);
    if (_logger) _logger->error(msg);
    return false;
}

bool SensorReader::initSGP(int maxAttempts, int delayBetweenMs) {
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        if (sgp.begin(&_wireSGP)) {
            if (attempt > 1) {
                String msg = "SGP40 initialized after " + String(attempt) + " attempts";
                Serial.println(msg);
                if (_logger) _logger->warn(msg);
            } else {
                String msg = "SGP40 initialized successfully";
                Serial.println(msg);
                if (_logger) _logger->info(msg);
            }
            return true;
        }
        if (attempt < maxAttempts) {
            delay(delayBetweenMs);
        }
    }
    String err = "Sensor SGP40 not found after retries :(";
    Serial.println(err);
    if (_logger) _logger->error(err);
    return false;
}

bool SensorReader::initSGP30(int maxAttempts, int delayBetweenMs) {
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        if (sgp30.begin(&_wireSGP)) {
            if (attempt > 1) {
                String msg = "SGP30 initialized after " + String(attempt) + " attempts";
                Serial.println(msg);
                if (_logger) _logger->warn(msg);
            } else {
                String msg = "SGP30 initialized successfully";
                Serial.println(msg);
                if (_logger) _logger->info(msg);
            }
            return true;
        }
        if (attempt < maxAttempts) {
            delay(delayBetweenMs);
        }
    }
    String err = "Sensor SGP30 not found after retries :(";
    Serial.println(err);
    if (_logger) _logger->error(err);
    return false;
}

bool SensorReader::initSPS30(int maxAttempts, int delayBetweenMs) {
    sps30Serial.begin(115200, SERIAL_8N1, 13, 27);
    sps30.begin(sps30Serial);

    int16_t ret;
    int8_t serialNumber[32];
    uint16_t serialNumberSize = 32;

    int attempts = 0;
    while (attempts < maxAttempts) {
        // 1. Hard Reset Sequence
        sps30.wakeUp(); 
        sps30.stopMeasurement(); 
        delay(100);
        sps30.deviceReset();
        delay(1000); // 1s for reboot

        // 2. Read Serial (Verify connection)
        ret = sps30.readSerialNumber(serialNumber, serialNumberSize);
        
        if (ret == 0) {
            String msg = "SPS30 detected, Serial: " + String((char*)serialNumber);
            Serial.println(msg);
            if (_logger) _logger->success(msg);
            
            // 3. Start Measurement
            ret = sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT);
            if (ret != 0 && ret != 1347 && (ret & 0xFF00) != 0x0500) {
                 String warn = "SPS30 Start failed: Err " + String(ret);
                 Serial.println(warn);
                 // Don't return false yet, retry loop will continue
            } else {
                 if (ret == 0) {
                     Serial.println("SPS30 Start Command Accepted.");
                 } else {
                     Serial.println("SPS30 Already Running.");
                 }
                 
                 // 4. VERIFY DATA: Wait 2s and try to read ONCE to confirm success
                 delay(2000); 
                 float p1, p2, p4, p10;
                 if (readSPS30(p1, p2, p4, p10)) {
                      String success = "SPS30 Initial Read Successful!";
                      Serial.println(success);
                      if (_logger) _logger->success(success);
                      return true; // FULL SUCCESS
                 } else {
                      String fail = "SPS30 Start looked OK but Read failed!";
                      Serial.println(fail);
                      if (_logger) _logger->error(fail);
                      // Loop will retry...
                 }
            }
        } else {
            Serial.println("SPS30 Serial Check Failed (Err " + String(ret) + ")");
        }
        attempts++;
        delay(delayBetweenMs);
    }
    
    String err = "SPS30 UART communication failed (Sensor not found)";
    Serial.println(err);
    if (_logger) _logger->error(err);
    return false;
}

int SensorReader::scanI2C(TwoWire& wire, const char* busName) {
    String startMsg = String("Scanning I2C bus: ") + busName + "...";
    Serial.println(startMsg);
    if (_logger) _logger->info(startMsg);

    byte error, address;
    int nDevices = 0;

    for(address = 1; address < 127; address++ ) {
        wire.beginTransmission(address);
        error = wire.endTransmission();

        if (error == 0) {
            String found = String("I2C device found at address 0x");
            if (address < 16) found += "0";
            found += String(address, HEX);
            found += " on " + String(busName);
            Serial.println(found);
            if (_logger) _logger->success(found);
            nDevices++;
        }
        else if (error == 4) {
             String err = String("Unknown error at address 0x");
             if (address < 16) err += "0";
             err += String(address, HEX);
             Serial.println(err);
             if (_logger) _logger->debug(err);
        }
    }
    
    if (nDevices == 0) {
        String msg = "No I2C devices found on " + String(busName);
        Serial.println(msg);
        if (_logger) _logger->warn(msg);
    } else {
        String msg = "Scan complete on " + String(busName) + ", devices: " + String(nDevices);
        Serial.println(msg);
        if (_logger) _logger->info(msg);
    }
    return nDevices;
}

bool SensorReader::readSPS30(float& pm1, float& pm25, float& pm4, float& pm10) {
    float mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typPartSize;
    int16_t ret;
    
    // Retry up to 3 times to read data before declaring failure
    for (int i = 0; i < 3; i++) {
        ret = sps30.readMeasurementValuesFloat(mc1p0, mc2p5, mc4p0, mc10p0, 
                                               nc0p5, nc1p0, nc2p5, nc4p0, nc10p0, typPartSize);
        if (ret == 0) {
            pm1 = mc1p0;
            pm25 = mc2p5;
            pm4 = mc4p0;
            pm10 = mc10p0;
            return true;
        }
        
        // If failed, wait a bit and clear buffer
        delay(100);
    }

    // If we reached here, 3 attempts failed
    String err = "SPS30 read failed after 3 attempts (Last Err: " + String(ret) + ")";
    Serial.println(err);
    if (_logger) _logger->error(err);

    // Auto-recovery: 
    // If read fails (Timeout 518), the sensor might have reset or is sleeping.
    sps30.wakeUp(); 
    
    int16_t startRet = sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT);
    if (startRet == 0) {
        String msg = "SPS30 restarted measurement (Auto-recovery)";
        Serial.println(msg);
        if (_logger) _logger->warn(msg);
    } else if (startRet == 1347 || (startRet & 0xFF00) == 0x0500) {
            // Already running, maybe just a hiccup?
            String msg = "SPS30 was already running during recovery (Error " + String(startRet) + ")";
            Serial.println(msg);
            if (_logger) _logger->warn(msg);
    } else {
            String msg = "SPS30 recovery failed: Start Error " + String(startRet);
            Serial.println(msg);
            if (_logger) _logger->error(msg);
    }
    
    return false;
}

bool SensorReader::resetBMP() {
    // 1. Try Soft Reset first
    Wire.beginTransmission(0x76);
    Wire.write(0xE0);
    Wire.write(0xB6);
    byte error = Wire.endTransmission();
    
    delay(100); 

    if (error == 0 && bmp.begin(0x76)) {
        String ok = "BMP280 soft reset successful";
        Serial.println(ok);
        if (_logger) _logger->success(ok);
        return true;
    }

    String msg = "Soft reset failed. Performing full I2C bus recovery...";
    Serial.println(msg);
    if (_logger) _logger->warn(msg);
    
    recoverI2C(21, 22);
    
    // Retry soft reset
    Wire.beginTransmission(0x76);
    Wire.write(0xE0);
    Wire.write(0xB6);
    Wire.endTransmission();
    
    delay(100);
    
    if (bmp.begin(0x76)) {
        String ok = "BMP280 sensor reset successful (after I2C recovery)";
        Serial.println(ok);
        if (_logger) _logger->success(ok);
        return true;
    }
    
    String err = "Failed to reset BMP280 sensor even after I2C recovery!";
    Serial.println(err);
    if (_logger) _logger->error(err);
    return false;
}

bool SensorReader::resetSGP() {
    bool success = sgp.begin(&_wireSGP);
    if (!success) {
        String err = "Failed to reset SGP40 sensor!";
        Serial.println(err);
        if (_logger) _logger->error(err);
        // Try to recover Wire1 (32, 33)
        recoverI2C(32, 33);
    } else {
        String ok = "SGP40 sensor reset successful";
        Serial.println(ok);
        if (_logger) _logger->success(ok);
    }
    return success;
}

void SensorReader::recoverI2C(int sdaPin, int sclPin) {
    String logMsg = "Recovering I2C bus on SDA=" + String(sdaPin) + " SCL=" + String(sclPin) + "...";
    Serial.println(logMsg);
    if (_logger) _logger->warn(logMsg);

    // Note: We cannot rely on Wire.end() for arbitrary pins if Wire is global instance, 
    // but here we are just toggling pins manually.
    
    // Explicitly set pins as input first to check state (optional)
    pinMode(sdaPin, INPUT);
    pinMode(sclPin, INPUT);
    delayMicroseconds(5);

    pinMode(sclPin, OUTPUT);
    digitalWrite(sclPin, LOW);
    pinMode(sdaPin, INPUT); 

    // Toggle SCL to release slave
    for (int i = 0; i < 9; i++) {
        digitalWrite(sclPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(sclPin, LOW);
        delayMicroseconds(10);
    }

    // Generate STOP condition
    pinMode(sdaPin, OUTPUT);
    digitalWrite(sdaPin, LOW);
    delayMicroseconds(10);
    digitalWrite(sclPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(sdaPin, HIGH);
    delayMicroseconds(10);

    pinMode(sdaPin, INPUT);
    pinMode(sclPin, INPUT);
    delayMicroseconds(5);

    // Re-init Wire instances is handled by caller (e.g. sgp.begin) 
    // OR we should re-begin here? 
    // Wire.begin(sda, scl) might be needed if we called Wire.end()?
    // But Wire1 is separate. 
    // To be safe, we just bang the pins. The next .begin() call from driver should take over.
    // However, for Wire (main), we did this:
    if (sdaPin == 21 && sclPin == 22) {
        Wire.begin(sdaPin, sclPin);
        Wire.setTimeOut(1000);
    } else {
        // Assume Wire1 
        // We can't access Wire1 global here easily without passing it or assuming standard pins?
        // _wireSGP is passed in constructor. We could use it:
        // _wireSGP.begin(sdaPin, sclPin);
        // But let's leave it to the next init call.
    }
    delay(100);
}

void SensorReader::resetDHT() {
    dht.begin();
    if (_logger) _logger->info("DHT reset");
}

void SensorReader::resetCO2() {
    while (co2Serial.available()) co2Serial.read();
    if (_logger) _logger->info("CO2 cleared");
}

bool SensorReader::isSGPConnected() {
    _wireSGP.beginTransmission(0x59);
    return _wireSGP.endTransmission() == 0;
}

bool SensorReader::isSGP30Connected() {
    _wireSGP.beginTransmission(0x58);
    return _wireSGP.endTransmission() == 0;
}

bool SensorReader::isBMPConnected() {
    Wire.beginTransmission(0x76);
    return Wire.endTransmission() == 0;
}

bool SensorReader::readSGP30(int& eco2, int& tvoc) {
    if (!isSGP30Connected()) return false;
    
    // Note: SGP30 needs measure() called every 1s ideally for baseline algo
    if (!sgp30.IAQmeasure()) {
        if (_logger) _logger->error("SGP30 read failed");
        return false;
    }

    eco2 = sgp30.eCO2;
    tvoc = sgp30.TVOC;
    return true;
}

int SensorReader::readVocIndex() {
    if (!isSGPConnected()) return -1;

    sensors_event_t temp, humidity;
    dht.temperature().getEvent(&temp);
    dht.humidity().getEvent(&humidity);
    
    float t = isnan(temp.temperature) ? 25.0 : temp.temperature;
    float h = isnan(humidity.relative_humidity) ? 50.0 : humidity.relative_humidity;
    
    return sgp.measureVocIndex(t, h);
}

float SensorReader::readPressure() {
    if (!isBMPConnected()) return NAN;
    return bmp.readPressure() / 100.0F;
}

float SensorReader::readBMPTemperature() {
    if (!isBMPConnected()) return NAN;
    return bmp.readTemperature();
}

int SensorReader::readCO2() {
    while (co2Serial.available()) co2Serial.read(); 

    co2Serial.write(CO2_READ_CMD, 9);
    unsigned long start = millis();
    while (co2Serial.available() < 9 && millis() - start < 500) {
        delay(10);
    }

    if (co2Serial.available() < 9) return -1; 

    uint8_t buf[9];
    co2Serial.readBytes(buf, 9);

    if (buf[0] != 0xFF || buf[1] != 0x86) return -2; 
    
    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) checksum += buf[i];
    checksum = 0xFF - checksum + 1;
    
    if (checksum != buf[8]) return -4; 

    int ppm = buf[2] * 256 + buf[3];
    return (ppm >= 0 && ppm <= 10000) ? ppm : -3;
}

DhtReading SensorReader::readDhtSensors() {
    DhtReading reading = {0.0, 0.0, false};
    sensors_event_t event;

    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
        reading.temperature = event.temperature;
        reading.valid = true;
    }
    
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
        reading.humidity = event.relative_humidity;
    }
    
    return reading;
}

bool SensorReader::initSHT(int maxAttempts, int delayBetweenMs) {
    // Force 100kHz and increase timeout for stability
    _wireSGP.setClock(100000);
    _wireSGP.setTimeOut(150);

    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        if (sht.begin(0x44)) {
            sht.reset(); // Soft reset to ensure clean state
            delay(100);  // Wait for reset
            
            if (attempt > 1) {
                String msg = "SHT31 initialized after " + String(attempt) + " attempts";
                Serial.println(msg);
                if (_logger) _logger->warn(msg);
            } else {
                String msg = "SHT31 initialized successfully (100kHz)";
                Serial.println(msg);
                if (_logger) _logger->info(msg);
            }
            return true;
        }
        if (attempt < maxAttempts) {
            String retry = "SHT31 init attempt " + String(attempt) + " failed, retrying...";
            Serial.println(retry);
            delay(delayBetweenMs);
        }
    }
    String err = "Sensor SHT31 not found after retries :(";
    Serial.println(err);
    if (_logger) _logger->error(err);
    return false;
}

bool SensorReader::isSHTConnected() {
    _wireSGP.beginTransmission(0x44);
    return _wireSGP.endTransmission() == 0;
}

bool SensorReader::readSHT(float& temp, float& hum) {
    if (!isSHTConnected()) return false;
    
    // Ensure accurate timing
    _wireSGP.setClock(100000); 

    // Read with retry and aggressive recovery
    for (int i = 0; i < 3; i++) {
        if (sht.readBoth(&temp, &hum)) {
            // Validate humidity
            if (hum >= 0 && hum <= 100 && temp > -45 && temp < 130) {
                 return true;
            }
        }
        
        // If read failed, try to recover by re-initializing
        if (i < 2) { 
            if (_logger) _logger->warn("SHT31 read failed, re-initializing...");
            if (sht.begin(0x44)) {
                delay(10); // Give it a moment after init
            } else {
                delay(50);
            }
        }
    }
    
    if (_logger) _logger->error("SHT31 read failed after 3 retries");
    return false;
}

void SensorReader::resetSHT() {
    if (_logger) _logger->warn("Resetting SHT3x sensor...");
    // Try to re-initialize
    if (initSHT()) {
        if (_logger) _logger->success("SHT3x reset successful");
    } else {
        if (_logger) _logger->error("SHT3x reset failed");
        // Try to recover Wire1 (32, 33)
        recoverI2C(32, 33);
    }
}
