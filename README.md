# Technical Documentation - ESP32 Air Quality Module

This document details the operation, wiring, and API of the ESP32 firmware for the air quality sensor.

## 🔌 Hardware Wiring

The ESP32 uses two distinct I2C buses to isolate sensors and prevent conflicts during resets.

| Component | Interface | ESP32 Pins | Notes |
|-----------|-----------|------------|-------|
| **BMP280** (Pressure/Temp) | I2C (Bus 0) | **SDA: 21, SCL: 22** | Main bus. Address `0x76`. |
| **SGP40** (VOC) | I2C (Bus 1) | **SDA: 32, SCL: 33** | **Secondary bus**. Default address. |
| **DHT22** (Temp/Hum) | 1-Wire | **Pin 4** | Pull-up req. |
| **MH-Z14A** (CO2) | UART (Serial2) | **RX: 16, TX: 17** | 9600 baud. |

> **Important Note**: The SGP40 **MUST** be on the secondary bus (32/33) to avoid being impacted by the main bus recovery procedures for the BMP280.

## ⚡ Power Supply

If powering the ESP32 via an external 5V power supply (instead of USB):
1.  Connect **5V Source (+) -> ESP32 VIN (or 5V) pin**.
2.  Connect **5V Source (-) -> ESP32 GND**.
3.  **Sensor Voltage Requirements**:
    *   **MH-Z14A (CO2)**: Requires **5V**. Connect its VCC to the **VIN** pin (using the direct 5V line). It **cannot** run on 3.3V.
    *   **BMP280, SGP40, DHT22**: Connect their VCC to the **ESP32 3V3** pin (regulated 3.3V).

---

## 🔄 Sensor Reset Strategy

The system implements a robustness strategy to handle sensor errors without restarting the entire ESP32.

### Reset Command
MQTT Topic: `{moduleId}/sensors/reset`
Payload: `{ "sensor": "<sensor_name>" }`

### Internal Logic

1.  **BMP280 (Pressure)**:
    *   **Attempt 1 (Soft Reset)**: Sends a software command to register `0xE0`. This does not disturb the I2C bus.
    *   **Attempt 2 (Hard I2C Recovery)**: If the soft reset fails, the firmware cuts and restarts the main I2C bus (Pins 21/22).
    *   *Thanks to the dual I2C bus, this "nuclear" action does not affect the SGP40.*

2.  **SGP40 (VOC)**:
    *   Standard reset via the Adafruit library on the secondary bus (`Wire1`).

3.  **DHT22**:
    *   Re-call of `dht.begin()`.

4.  **MH-Z14A (CO2)**:
    *   Clearing of the Serial buffer (UART).

---

## 📡 MQTT API

### Published Topics

| Topic | Frequency | Description |
|-------|-----------|-------------|
| `{moduleId}/temperature` | Configurable | Temperature (°C) via DHT22 |
| `{moduleId}/humidity` | Configurable | Humidity (%) via DHT22 |
| `{moduleId}/pressure` | Configurable | Pressure (hPa) via BMP280 |
| `{moduleId}/temperature_bmp` | Configurable | Internal BMP280 Temperature |
| `{moduleId}/co2` | Configurable | CO2 (ppm) via MH-Z14A |
| `{moduleId}/voc` | Configurable | VOC Index (0-500) via SGP40 |
| `{moduleId}/sensors/status` | Event / 5s | Detailed status and latest values |
| `{moduleId}/system` | 5s | System info (IP, RSSI, Memory) |

### JSON Status Structure

**Topic**: `{moduleId}/sensors/status`
```json
{
  "co2": { "status": "ok", "value": 450 },
  "temperature": { "status": "ok", "value": 22.5 },
  "voc": { "status": "ok", "value": 110 },
  "pressure": { "status": "error", "value": "null" }
}
```

---

## 💾 Firmware Architecture

The code is structured into C++ modules for maintainability:

*   **`AppController`**: Orchestrator, initializes subsystems and dispatches tasks.
*   **`SensorReader`**: Hardware abstraction. Manages library instances (`Adafruit_BMP280`, etc.) and I2C buses.
*   **`NetworkManager`**: Manages WiFi connection (with auto-reconnect) and MQTT client.
*   **`MqttHandler`**: Receives and parses incoming messages (reset commands, config).
*   **`RemoteLogger`**: Sends system logs via MQTT for remote debugging.
