# Cablage et Pinout ESP32 (Air Quality Monitor)

Documentation des connexions matérielles pour le module ESP32.

## Résumé du Pinout

| PIN (ESP32) | Fonction | Connecté à | Notes |
| :--- | :--- | :--- | :--- |
| **2** | UART1 RX | MH-Z19/14A (TX) | CO2 (Serial1, mais re-mappé sur 25/26 dans `SystemInitializer`?? Vérifier code) |
| **25** | UART2 RX | MH-Z14A (TX) | CO2 (Voir `SystemInitializer.cpp`) |
| **26** | UART2 TX | MH-Z14A (RX) | CO2 (Voir `SystemInitializer.cpp`) |
| **21** | I2C SDA | BMP280 (SDA) | Bus Principal (Wire) |
| **22** | I2C SCL | BMP280 (SCL) | Bus Principal (Wire) |
| **32** | I2C SDA | SGP40 & **SGP30** | Bus Secondaire (Wire1) |
| **33** | I2C SCL | SGP40 & **SGP30** | Bus Secondaire (Wire1) |
| **4** | DATA | DHT22 | Temp/Humidité |
| **13** | UART RX | SPS30 (TX) | Particules Fines |
| **27** | UART TX | SPS30 (RX) | Particules Fines |
| **VIN/5V** | Power | Tous capteurs (sauf SGP30/40 en 3.3V?) | Vérifier voltage requis par module |
| **GND** | Ground | Tous capteurs | Masse commune |

## Instructions de Connexion SGP30 (Nouveau)

Le SGP30 doit être connecté sur le **Second Bus I2C**, en parallèle avec le SGP40 existant.

**Cablage :**
1.  **VCC** -> 3.3V (ou 5V si votre module le gère)
2.  **GND** -> GND
3.  **SDA** -> Pin **32** (Connecter AVEC le fil SDA du SGP40)
4.  **SCL** -> Pin **33** (Connecter AVEC le fil SCL du SGP40)

> [!TIP]
> **Adresses I2C :**
> - SGP30 : `0x58`
> - SGP40 : `0x59`

---

## ⚠️ Avertissement Consommation (Power Budget)

Vous avez beaucoup de capteurs, dont certains consomment beaucoup (pics de courant).
Un port USB standard (PC) délivre **500mA** max.

**Estimation de consommation (Pic) :**
- **ESP32 (WiFi TX)** : ~260 mA
- **MH-Z14A (Chauffage)** : ~150 mA
- **SPS30 (Ventilateur/Start)** : ~80-100 mA
- **SGP30 (Chauffage)** : ~48 mA
- **SGP40 + BMP + DHT** : ~10 mA
- **TOTAL PIC** : **~650 mA** 🚨

**Risque :** "Brownout" (chute de tension), redémarrages intempestifs de l'ESP32, ou capteurs qui échouent (Timeout/Error).

**Recommandation :**
Utilisez une alimentation externe USB solide (**2A minimum**, type chargeur de téléphone) connectée à l'ESP32, plutôt que le port USB de l'ordinateur si vous constatez des instabilités.
