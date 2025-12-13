#ifndef SYSTEM_INFO_COLLECTOR_H
#define SYSTEM_INFO_COLLECTOR_H

#include <Arduino.h>
#include "SensorData.h"

class SystemInfoCollector {
public:
    static SystemInfo collect();
    static String buildPsramJson();
};

#endif

