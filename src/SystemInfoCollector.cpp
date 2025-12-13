#include "SystemInfoCollector.h"

SystemInfo SystemInfoCollector::collect() {
    SystemInfo info;
    info.flashTotal = ESP.getFlashChipSize() / 1024;
    info.flashUsed = (ESP.getSketchSize() + ESP.getSketchMD5().length() + 0x1000) / 1024;
    info.flashFree = (ESP.getFreeSketchSpace() - 0x1000) / 1024;
    info.flashSystemPartitions = info.flashTotal - info.flashUsed - info.flashFree;
    info.heapTotal = ESP.getHeapSize() / 1024;
    info.heapFree = ESP.getFreeHeap() / 1024;
    info.heapMinFree = ESP.getMinFreeHeap() / 1024;
    info.heapUsedPercent = ((float)(ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) * 100.0;
    return info;
}

String SystemInfoCollector::buildPsramJson() {
    if (ESP.getPsramSize() == 0) return "";
    
    float psramUsedPercent = ((float)(ESP.getPsramSize() - ESP.getFreePsram()) / ESP.getPsramSize()) * 100.0;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), 
        ",\\\"psram\":{"
            "\\\"totalKb\\\":%d,"
            "\\\"freeKb\\\":%d,"
            "\\\"usedPercent\\\":%.1f"
        "}",
        ESP.getPsramSize() / 1024,
        ESP.getFreePsram() / 1024,
        psramUsedPercent
    );
    return String(buffer);
}
