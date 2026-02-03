# ESP8266 (D1 mini) - Capteurs vers MQTT (DHT22 + BMP388 + LDR + PIR)

Ce projet réalise une petite station de capteurs basée sur une **D1 mini (ESP8266)**.  
Les mesures sont publiées sur un **broker MQTT Mosquitto** et peuvent être visualisées sur smartphone (ex. : application **IoT MQTT Panel**).

## Fonctionnalités

- Lecture des capteurs :
  - **DHT22** : température (°C) + humidité (%)
  - **BMP388 (I2C)** : température (°C), pression (mbar), altitude (m)
  - **LDR + résistance 10k** sur **A0** : luminosité (valeur ADC brute)
  - **PIR HC-SR501** : mémorise la date/heure du dernier mouvement détecté
- Publication MQTT toutes les 1 s
- Serveur HTTP local (page HTML) sur `http://<ip_esp8266>/`
- Synchronisation NTP + fuseau **Europe/Paris** avec gestion automatique **heure d’été / heure d’hiver**

## Matériel

- 1 × D1 mini (ESP8266)
- 1 × DHT22
- 1 × BMP388 (I2C)
- 1 × LDR (photorésistance) + **résistance 10k**
- 1 × PIR **HC-SR501** (alimentation **5V**)
- Fils + plaque à pastilles / breadboard
- Alimentation USB

## Câblage (résumé)

### I2C (BMP388)
- **SDA = D2 (GPIO4)**
- **SCL = D1 (GPIO5)**
- VCC = 3V3
- GND = GND

### DHT22
- DATA = **D7 (GPIO13)**
- VCC = 3V3
- GND = GND

### LDR (luminosité)
- Entrée analogique : **A0**
- Pont diviseur classique (exemple) :
  - 3V3 → LDR → A0 → **10k** → GND

> Remarque : selon le montage, la valeur ADC peut monter ou descendre quand il fait plus clair.  
> Dans tous les cas, la valeur publiée reste valide comme **mesure brute**.

### PIR HC-SR501 (mouvement)
- OUT = **D6 (GPIO12)**
- VCC = **5V**
- GND = GND

> Le HC-SR501 a souvent un temps de stabilisation au démarrage (quelques secondes).

## MQTT

Broker par défaut : `test.mosquitto.org` port `1883` (public, sans authentification).

### Topics publiés

- `capteurs/temperature` : température en °C
- `capteurs/humidite` : humidité en %
- `capteurs/pression` : pression en mbar (hPa)
- `capteurs/altitude` : altitude en mètres (m)
- `capteurs/luminosite` : valeur ADC brute (0..1023)
- `capteurs/date_heure` : date/heure locale `dd-mm-yyyy  hh:mm:ss`
- `capteurs/mouvement` : date/heure du dernier mouvement détecté

## IDE et bibliothèques

- Arduino IDE 1.8.19 (ou plus récent)
- Carte : ESP8266 (D1 mini)
- Bibliothèques :
  - PubSubClient
  - DHT sensor library
  - Adafruit BMP3XX
  - Adafruit Unified Sensor (dépendance)

## Serveur Web

Une page HTML simple est disponible :
- `http://<ip_esp8266>/`

Elle affiche les mesures courantes et l’horodatage.

## Captures / photos


![Montage](/docs/screenshots/montage.jpg)
![Iphone](/docs/screenshots/mqttpanel.jpg)



## Licence

MIT

