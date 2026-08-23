
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include "time.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#include <DHT.h>

// -------------------- WiFi (placeholders) --------------------
#define ssid      "SSID"
#define password  "MDP"

//delais
time_t lastEpoch = 0;
unsigned long lastTickMs = 0;
const unsigned long periodMs = 1000;


// -------------------- MQTT --------------------
const char* mqtt_server = "test.mosquitto.org"; // broker.emqx.io
const int   mqtt_port   = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP8266WebServer server(80);

// -------------------- BMP388 --------------------
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BMP3XX bmp;

// -------------------- DHT22 --------------------
#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// -------------------- LDR --------------------
const int photoR = 0;        // A0
int LecturephotoR = 0;

// -------------------- PIR --------------------
const int PIR_Out = 12;      // D6 (GPIO12)
bool pirStatus = false;

// -------------------- Mesures --------------------
float h = 0.0f;
float t = 0.0f;
float a = 0.0f;
float p = 0.0f;

// -------------------- Temps --------------------
time_t maintenant;
struct tm timeinfo;
char Maintenant[21];
char DatePir[21];

// -------------------- Buffers MQTT --------------------
char bufTemp[16];
char bufHum[16];
char bufPress[16];
char bufAlt[16];
char bufLum[16];

// -------------------- Helpers --------------------
static void formatDateTime(char* out, size_t outSize, const struct tm* ti) {
  // dd-mm-yyyy␠␠hh:mm:ss
  snprintf(out, outSize,
           "%02d-%02d-%04d  %02d:%02d:%02d",
           ti->tm_mday,
           ti->tm_mon + 1,
           ti->tm_year + 1900,
           ti->tm_hour,
           ti->tm_min,
           ti->tm_sec);
}

static void readSensors() {
  // BMP388: lecture robuste
  if (bmp.performReading()) {
    t = bmp.temperature;
    p = bmp.pressure / 100.0f;  // Pa -> hPa (mbar)
    a = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  } else {
    // On ne modifie pas t/p/a si la lecture echoue
    Serial.println("BMP388: lecture impossible");
  }

  // DHT22: protection NaN
  float hh = dht.readHumidity();
  if (!isnan(hh)) {
    h = hh;
  } else {
    Serial.println("DHT22: lecture humidite impossible");
  }

  // LDR
  LecturephotoR = analogRead(photoR);
}

static void publishMQTT() {
  // Conversions stables vers char*
  dtostrf(t, 0, 2, bufTemp);
  dtostrf(h, 0, 2, bufHum);
  dtostrf(p, 0, 2, bufPress);
  dtostrf(a, 0, 2, bufAlt);
  snprintf(bufLum, sizeof(bufLum), "%d", LecturephotoR);

  mqttClient.publish("capteurs/temperature", bufTemp);
  mqttClient.publish("capteurs/humidite", bufHum);
  mqttClient.publish("capteurs/pression", bufPress);
  mqttClient.publish("capteurs/altitude", bufAlt);
  mqttClient.publish("capteurs/luminosite", bufLum);
  mqttClient.publish("capteurs/date_heure", Maintenant);
  mqttClient.publish("capteurs/mouvement", DatePir);
}

// -------------------- Web page --------------------
static void handleRoot() {
  char page[900];

  // page HTML (sans String)
  // note: on limite la taille, mais 900 suffit ici.
  snprintf(page, sizeof(page),
           "<html lang='fr-FR'>"
           "<head>"
           "<meta charset='UTF-8'>"
           "<meta http-equiv='refresh' content='2'/>"
           "<title>Serveur Capteurs</title>"
           "<style>"
           "body{background-color:#FFFFFF;font-family:Arial,Helvetica,Sans-Serif;color:#000088;}"
           "</style>"
           "</head>"
           "<body>"
           "<h1>Serveur BUREAU DE PHILIPPE</h1>"
           "<ul>"
           "<li>Temperature : %s C</li>"
           "<li>Taux d'humidite : %s %%</li>"
           "<li>Pression atmospherique : %s mbar</li>"
           "<li>Altitude (BMP388) : %s m</li>"
           "<li>Luminosite : %s</li>"
           "<li>Dernier mouvement detecte : %s</li>"
           "<li>Date/Heure : %s</li>"
           "</ul>"
           "</body>"
           "</html>",
           bufTemp, bufHum, bufPress, bufAlt, bufLum, DatePir, Maintenant);

  server.send(200, "text/html", page);
}

// -------------------- MQTT reconnect --------------------
static void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Tentative de connexion MQTT...");

    char clientId[32];
    unsigned long r = (unsigned long)random(0xffff);
    snprintf(clientId, sizeof(clientId), "ESP8266Client-%04lX", r);

    if (mqttClient.connect(clientId)) {
      Serial.println("connecte");
    } else {
      Serial.print("Echec, code erreur : ");
      Serial.println(mqttClient.state());
      Serial.println("Nouvelle tentative dans 5 secondes...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // DHT
  dht.begin();

  // PIR
  pinMode(PIR_Out, INPUT);

  // I2C (D1 mini: SDA=D2, SCL=D1)
  Wire.begin();

  // BMP388
  if (!bmp.begin_I2C()) {
    Serial.println("BMP388 KO!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("BMP388 OK");

  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connecte au reseau ");
  Serial.println(ssid);
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  // Web server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Serveur HTTP demarre");

  // Time + TZ (Europe/Paris DST)
  configTime(0, 0, "fr.pool.ntp.org", "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // Attente de la synchronisation NTP
  Serial.print("Attente NTP");
  while (true) {
    time(&maintenant);
    localtime_r(&maintenant, &timeinfo);

    if (timeinfo.tm_year >= 120) { // >= 2020
      break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK");

  formatDateTime(Maintenant, sizeof(Maintenant), &timeinfo);
  snprintf(DatePir, sizeof(DatePir), "%s", Maintenant);


  // MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();
}

void loop() {
  server.handleClient();

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  time(&maintenant);
  if (maintenant != lastEpoch) {
    lastEpoch = maintenant;

    // Convertir en heure locale (Europe/Paris)
    localtime_r(&maintenant, &timeinfo);
    formatDateTime(Maintenant, sizeof(Maintenant), &timeinfo);
    Serial.print("epoch=");
    Serial.print((unsigned long)maintenant);
    Serial.print("  time=");
    Serial.print(Maintenant);
    Serial.print("  ms=");
    Serial.println(millis());

    // Lecture capteurs
    readSensors();

    // PIR: memoriser dernier mouvement
    pirStatus = (digitalRead(PIR_Out) == HIGH);
    if (pirStatus) {
      snprintf(DatePir, sizeof(DatePir), "%s", Maintenant);
    }

    // Publier MQTT
    publishMQTT();
  }
}
