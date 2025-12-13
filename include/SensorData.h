#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

struct DhtReading {
    float temperature;
    float humidity;
    bool valid;
};

struct SensorData {
    float co2;
    float temperature;
    float humidity;
    float vocIndex;
    float pressure;
    float temperature_bmp;
    float pm1;
    float pm25;
    float pm4;
    float pm10;
    int eco2;
    int tvoc;
    float temperature_sht;
    float humidity_sht;
    
    // Status tracking (true if last read was successful)
    bool co2Valid;
};

struct SystemInfo {
    int flashTotal;
    int flashUsed;
    int flashFree;
    int flashSystemPartitions;
    int heapTotal;
    int heapFree;
    int heapMinFree;
    float heapUsedPercent;
};

struct SensorConfig {
    unsigned long co2Interval = 60000;
    unsigned long tempInterval = 60000;
    unsigned long humInterval = 60000;
    unsigned long vocInterval = 60000;
    unsigned long pressureInterval = 60000;
    unsigned long pmInterval = 60000;
    unsigned long eco2Interval = 60000;
    unsigned long tvocInterval = 60000;
    unsigned long shtInterval = 60000;
};

#endif
