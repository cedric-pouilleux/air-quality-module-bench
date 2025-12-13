# Câblage ESP32 - Module Qualité de l'Air

## 📌 Tableau des Connexions

| PIN ESP32 | Fonction | Capteur | Notes |
|-----------|----------|---------|-------|
| **GPIO 4** | DATA | DHT22 | 1-Wire, pull-up requis |
| **GPIO 21** | I2C0 SDA | BMP280 | Bus principal |
| **GPIO 22** | I2C0 SCL | BMP280 | Bus principal |
| **GPIO 32** | I2C1 SDA | SGP40, SGP30, SHT31 | Bus secondaire |
| **GPIO 33** | I2C1 SCL | SGP40, SGP30, SHT31 | Bus secondaire |
| **GPIO 25** | UART RX | MH-Z14A TX | CO2 |
| **GPIO 26** | UART TX | MH-Z14A RX | CO2 |
| **GPIO 13** | UART RX | SPS30 TX | Particules |
| **GPIO 27** | UART TX | SPS30 RX | Particules |
| **VIN (5V)** | Power | MH-Z14A | Requiert 5V |
| **3V3** | Power | Autres capteurs | 3.3V régulé |
| **GND** | Ground | Tous | Masse commune |

---

## 🔌 Adresses I2C

### Bus 0 (GPIO 21/22)
| Capteur | Adresse |
|---------|---------|
| BMP280 | `0x76` |

### Bus 1 (GPIO 32/33)
| Capteur | Adresse |
|---------|---------|
| SGP40 | `0x59` |
| SGP30 | `0x58` |
| SHT31 | `0x44` |

---

## ⚡ Alimentation

### Voltage par Capteur

| Capteur | Tension | Source |
|---------|---------|--------|
| MH-Z14A (CO2) | **5V** | VIN direct |
| SPS30 (PM) | **5V** | VIN direct |
| DHT22 | 3.3V - 5V | 3V3 ou VIN |
| BMP280 | 3.3V | 3V3 |
| SGP40 | 3.3V | 3V3 |
| SGP30 | 3.3V | 3V3 |
| SHT31 | 3.3V | 3V3 |

### Budget de Puissance

| Composant | Consommation (pic) |
|-----------|-------------------|
| ESP32 (WiFi TX) | ~260 mA |
| MH-Z14A (chauffage) | ~150 mA |
| SPS30 (ventilateur) | ~80-100 mA |
| SGP30 (chauffage) | ~48 mA |
| Autres | ~10 mA |
| **TOTAL** | **~650 mA** 🚨 |

> ⚠️ **Attention** : Un port USB standard (500mA) est insuffisant !
> Utilisez une alimentation 2A minimum.

---

## 🔧 Schéma de Câblage

```
ESP32                    Capteurs
┌──────────────┐
│              │
│     GPIO 4 ──┼──────── DHT22 (DATA)
│              │
│    GPIO 21 ──┼──┬───── BMP280 (SDA)
│    GPIO 22 ──┼──┴───── BMP280 (SCL)
│              │
│    GPIO 32 ──┼──┬───── SGP40/SGP30/SHT31 (SDA)
│    GPIO 33 ──┼──┴───── SGP40/SGP30/SHT31 (SCL)
│              │
│    GPIO 25 ──┼──────── MH-Z14A (TX)
│    GPIO 26 ──┼──────── MH-Z14A (RX)
│              │
│    GPIO 13 ──┼──────── SPS30 (TX)
│    GPIO 27 ──┼──────── SPS30 (RX)
│              │
│       VIN ───┼──────── MH-Z14A, SPS30 (5V)
│       3V3 ───┼──────── Autres capteurs (3.3V)
│       GND ───┼──────── Tous (GND)
│              │
└──────────────┘
```

---

## 📝 Notes

1. **Double Bus I2C** : Isole le BMP280 (instable) des capteurs SGP. Un reset du Bus 0 n'affecte pas le Bus 1.

2. **Pull-up DHT22** : Résistance 4.7kΩ - 10kΩ entre DATA et 3V3 recommandée.

3. **MH-Z14A** : Orientation des fils importante, vérifier TX/RX.

4. **SPS30** : Nécessite un connecteur JST-ZHR-5 ou câblage direct.
