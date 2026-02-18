#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <Adafruit_VEML6070.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// HDC1080 – Temperatur & Luftfeuchtigkeit
ClosedCube_HDC1080 hdc;

// VEML6070 – UV-Intensität
Adafruit_VEML6070 veml = Adafruit_VEML6070();

// DS18B20 – Temperatur im Bienenstock (wasserdichter Sensor)
// HINWEIS: Falls der Sensor nicht erkannt wird (-127°C), anderen Pin testen
// oder den DS18B20_Pin_Scanner.ino verwenden!
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DeviceAddress ds18b20Address;       // Geräteadresse für gezielte Abfragen
bool ds18b20Found = false;          // ob ein gültiger Sensor gefunden wurde

/* ------------------------------------------------------------------------- */
/* ---------------------------------Metadata-------------------------------- */
/* ------------------------------------------------------------------------- */
/* SENSEBOX ID  : 6994ab6bb3ae3d000839d743                                   */
/* SENSEBOX NAME: BeeSense                                                   */
/* ------------------------------------------------------------------------- */
/* ------------------------------End of Metadata---------------------------- */
/* ------------------------------------------------------------------------- */

// senseBox ID
#define SENSEBOX_ID "6994ab6bb3ae3d000839d743"

// Sensor IDs
// Temperatur - HDC1080
#define SENSOR1_ID "6994ab6bb3ae3d000839d744"
// rel. Luftfeuchte - HDC1080
#define SENSOR2_ID "6994ab6bb3ae3d000839d745"
// UV-Intensität - VEML6070
#define SENSOR3_ID "6994ab6bb3ae3d000839d746"
// Temperatur inside Beehive - DS18B20
#define SENSOR4_ID "6994ab6bb3ae3d000839d748"

// Messparameter
const unsigned int postingInterval = 60000; // Uploadintervall in Millisekunden

// DS18B20 Fehlerwerte
#define DS18B20_DISCONNECTED -127.0
#define DS18B20_RESET_VALUE   85.0

// HDC1080 Fehlerwerte
#define HDC1080_ERROR_TEMP   125.0
#define HDC1080_MAX_RETRIES  3

// Maximale Versuche beim Lesen des DS18B20
#define DS18B20_MAX_RETRIES 3

// I2C-Sensor Status (Auto-Erkennung)
bool hdc1080Found = false;
bool veml6070Found = false;

/* ---- DS18B20 Hilfsfunktionen ---- */

// Gibt die OneWire-Adresse als HEX-String aus
void printDS18B20Address(DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i < 7) Serial.print(":");
  }
}

// Initialisiert den DS18B20 und sucht nach Geräten
// Gibt true zurück falls ein Sensor gefunden wurde
bool initDS18B20() {
  ds18b20.begin();
  ds18b20.setWaitForConversion(true);  // Blockiert bis Konvertierung fertig
  ds18b20.setResolution(12);           // Höchste Auflösung (0.0625°C)

  int deviceCount = ds18b20.getDeviceCount();
  Serial.print("INFO: DS18B20 device count: ");
  Serial.println(deviceCount);

  if (deviceCount == 0) {
    Serial.println("WARN: Kein DS18B20 Sensor auf Pin ");
    Serial.print(ONE_WIRE_BUS);
    Serial.println(" gefunden!");
    Serial.println("WARN: Pruefe Verkabelung: DATA -> Pin, 4.7kOhm Pull-Up zwischen DATA und VCC");
    Serial.println("WARN: Nutze DS18B20_Pin_Scanner.ino um den richtigen Pin zu finden");
    return false;
  }

  // Erste Geräteadresse auslesen
  if (ds18b20.getAddress(ds18b20Address, 0)) {
    Serial.print("INFO: DS18B20 Adresse: ");
    printDS18B20Address(ds18b20Address);
    Serial.println();

    // Gerätetyp prüfen
    Serial.print("INFO: DS18B20 Typ: ");
    switch (ds18b20Address[0]) {
      case 0x10: Serial.println("DS18S20"); break;
      case 0x28: Serial.println("DS18B20"); break;
      case 0x22: Serial.println("DS1822"); break;
      default:   Serial.println("Unbekannt"); break;
    }

    Serial.print("INFO: Parasite Power: ");
    Serial.println(ds18b20.isParasitePowerMode() ? "Ja" : "Nein");

    return true;
  }

  Serial.println("WARN: Konnte DS18B20 Adresse nicht lesen");
  return false;
}

// Liest den DS18B20 mit Retry-Logik
// Auf ESP32-S3: I2C wird suspendiert, damit OneWire-Timing nicht gestört wird
float readDS18B20WithRetry() {
  // I2C suspendieren – verhindert Interrupt-Konflikte mit OneWire-Timing
  Wire.end();
  delay(20);

  float result = DEVICE_DISCONNECTED_C;

  for (int attempt = 1; attempt <= DS18B20_MAX_RETRIES; attempt++) {
    // OneWire-Bus neu initialisieren (wie der Pin Scanner)
    oneWire.reset();
    delay(50);

    ds18b20.begin();  // Frische Initialisierung wie im Pin Scanner
    delay(50);

    ds18b20.requestTemperatures();
    delay(800);  // 12-Bit braucht bis zu 750ms

    float temp;
    if (ds18b20Found) {
      temp = ds18b20.getTempC(ds18b20Address);
    } else {
      temp = ds18b20.getTempCByIndex(0);
    }

    // Gültigkeitsprüfung
    if (temp != DEVICE_DISCONNECTED_C && temp > -50.0 && temp < 125.0) {
      if (temp == DS18B20_RESET_VALUE && attempt < DS18B20_MAX_RETRIES) {
        Serial.println("WARN: DS18B20 liefert 85.0°C (Reset-Wert), neuer Versuch...");
        delay(200);
        continue;
      }
      result = temp;
      break;
    }

    Serial.print("WARN: DS18B20 Leseversuch ");
    Serial.print(attempt);
    Serial.print("/");
    Serial.print(DS18B20_MAX_RETRIES);
    Serial.print(" fehlgeschlagen (Wert: ");
    Serial.print(temp);
    Serial.println(" °C)");

    if (attempt < DS18B20_MAX_RETRIES) {
      delay(250);
    }
  }

  // I2C wieder aktivieren
  Wire.begin();
  delay(20);

  return result;
}

/* ---- HDC1080 Hilfsfunktion ---- */

// Liest HDC1080 Temperatur mit Retry (125°C = typischer I2C-Fehlerwert)
float readHDC1080TempWithRetry() {
  for (int attempt = 1; attempt <= HDC1080_MAX_RETRIES; attempt++) {
    float temp = hdc.readTemperature();
    if (temp != HDC1080_ERROR_TEMP && temp > -40.0 && temp < 85.0) {
      return temp;
    }
    Serial.print("WARN: HDC1080 Temperatur Versuch ");
    Serial.print(attempt);
    Serial.print("/");
    Serial.print(HDC1080_MAX_RETRIES);
    Serial.print(" fehlgeschlagen (Wert: ");
    Serial.print(temp);
    Serial.println(" °C)");
    delay(200);
  }
  return HDC1080_ERROR_TEMP;  // Alle Versuche fehlgeschlagen
}

/* ---- Setup & Loop ---- */

void setup () {
  Serial.begin(115200);

  // Warte kurz auf Serial-Verbindung
  delay(2000);

  Serial.println("INFO: BeeSense starting... (ESP32-S3)");
  Serial.print("INFO: DS18B20 Pin: GPIO ");
  Serial.println(ONE_WIRE_BUS);

  // =====================================================
  // SCHRITT 1: DS18B20 ZUERST initialisieren (OHNE I2C!)
  // Auf ESP32-S3 stört Wire.begin() das OneWire-Timing.
  // =====================================================
  Serial.println("INFO: [Phase 1] DS18B20 Init (vor I2C)...");

  // OneWire-Pin konfigurieren
  pinMode(ONE_WIRE_BUS, INPUT);
  delay(100);

  // OneWire Reset
  Serial.println("INFO: OneWire Reset...");
  oneWire.reset();
  delay(500);

  ds18b20Found = initDS18B20();

  // Erste Aufwärm-Messung verwerfen
  if (ds18b20Found) {
    Serial.println("INFO: DS18B20 Aufwaermmessung...");
    ds18b20.requestTemperatures();
    delay(1000);
    float warmup = ds18b20.getTempCByIndex(0);
    Serial.print("INFO: DS18B20 Aufwaermmessung Ergebnis: ");
    Serial.print(warmup);
    Serial.println(" °C (wird verworfen)");
  }

  // Falls nicht gefunden: mehrere Versuche
  if (!ds18b20Found) {
    Serial.println("INFO: Versuche OneWire Reset-Pulse...");
    for (int retry = 0; retry < 3; retry++) {
      delay(1000);
      oneWire.reset();
      delay(250);
      ds18b20Found = initDS18B20();
      if (ds18b20Found) {
        Serial.println("INFO: DS18B20 nach erneutem Versuch gefunden!");
        // Aufwärm-Messung
        ds18b20.requestTemperatures();
        delay(1000);
        ds18b20.getTempCByIndex(0);  // verwerfen
        break;
      }
    }
  }

  if (!ds18b20Found) {
    Serial.println("ERROR: DS18B20 konnte nicht initialisiert werden!");
    Serial.println("ERROR: Bitte Verkabelung pruefen und DS18B20_Pin_Scanner.ino verwenden");
  }

  // =====================================================
  // SCHRITT 2: I2C-Sensoren initialisieren (NACH OneWire)
  // =====================================================
  Serial.println("INFO: [Phase 2] I2C Sensoren Init...");
  delay(500);

  Wire.begin();
  delay(100);

  // HDC1080 erkennen: Teste I2C-Adresse 0x40
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) {
    hdc1080Found = true;
    hdc.begin(0x40);
    delay(100);
    float testTemp = hdc.readTemperature();
    Serial.print("INFO: HDC1080 gefunden, Test-Temperatur: ");
    Serial.print(testTemp);
    Serial.println(" °C");
  } else {
    hdc1080Found = false;
    Serial.println("INFO: HDC1080 nicht angeschlossen (wird uebersprungen)");
  }

  // VEML6070 erkennen: Teste I2C-Adresse 0x38
  Wire.beginTransmission(0x38);
  if (Wire.endTransmission() == 0) {
    veml6070Found = true;
    veml.begin(VEML6070_1_T);
    delay(100);
    Serial.println("INFO: VEML6070 gefunden");
  } else {
    veml6070Found = false;
    Serial.println("INFO: VEML6070 nicht angeschlossen (wird uebersprungen)");
  }

  Serial.println("INFO: Sensors initialized.");
  Serial.println("INFO: Starting measurements...");
  Serial.println("INFO: Ready for data transmission");
}

void loop () {
  Serial.println("INFO: ===========================================");
  Serial.println("INFO: Reading sensors...");
  Serial.println("INFO: ===========================================");

  // HDC1080: Temperatur auslesen (nur wenn Sensor vorhanden)
  if (hdc1080Found) {
    float temperature = readHDC1080TempWithRetry();
    Serial.print("INFO: Temperature (HDC1080): ");
    Serial.print(temperature);
    Serial.println(" °C");
    if (temperature != HDC1080_ERROR_TEMP) {
      sendSensorData(temperature, SENSOR1_ID, "Temperature");
    } else {
      Serial.println("WARN: HDC1080 Temperatur ungueltig (125°C), wird NICHT gesendet");
    }

    delay(200);  // Pause zwischen I2C-Lesungen

    // HDC1080: rel. Luftfeuchtigkeit auslesen
    float humidity = hdc.readHumidity();
    Serial.print("INFO: Humidity (HDC1080): ");
    Serial.print(humidity);
    Serial.println(" %");
    sendSensorData(humidity, SENSOR2_ID, "Humidity");

    delay(200);  // Pause zwischen I2C-Geraten
  } else {
    Serial.println("INFO: HDC1080 nicht vorhanden, ueberspringe...");
  }

  // VEML6070: UV-Intensität auslesen (nur wenn Sensor vorhanden)
  if (veml6070Found) {
    uint16_t uvIntensity = veml.readUV();
    Serial.print("INFO: UV Intensity (VEML6070): ");
    Serial.println(uvIntensity);
    sendSensorData((float)uvIntensity, SENSOR3_ID, "UV_Intensity");

    delay(300);  // Pause vor Bus-Wechsel (I2C -> OneWire)
  } else {
    Serial.println("INFO: VEML6070 nicht vorhanden, ueberspringe...");
  }

  // DS18B20: Temperatur im Bienenstock auslesen
  // I2C wird intern suspendiert um OneWire-Timing nicht zu stoeren
  float beehiveTemperature = readDS18B20WithRetry();
  Serial.print("INFO: Beehive Temperature (DS18B20): ");
  Serial.print(beehiveTemperature);
  Serial.println(" °C");

  if (beehiveTemperature > -50.0 && beehiveTemperature < 125.0) {
    sendSensorData(beehiveTemperature, SENSOR4_ID, "Beehive_Temperature");
  } else {
    Serial.println("WARN: DS18B20 ungültiger Wert, wird NICHT gesendet");
    // Versuche Sensor erneut zu initialisieren für den nächsten Durchlauf
    Serial.println("INFO: Re-Initialisierung DS18B20...");
    ds18b20Found = initDS18B20();
  }

  Serial.println("INFO: ===========================================");
  Serial.print("INFO: Waiting ");
  Serial.print(postingInterval / 1000);
  Serial.println(" seconds...");
  Serial.println("INFO: ===========================================");
  Serial.println();

  delay(postingInterval);
}

// Sende Sensordaten im JSON-Format über Serial an die Bridge
// Format: DATA|{"boxId":"xxx","sensorId":"xxx","value":12.34,"type":"Temperature"}
void sendSensorData(float measurement, String sensorId, String sensorType) {
  Serial.print("DATA|{\"boxId\":\"");
  Serial.print(SENSEBOX_ID);
  Serial.print("\",\"sensorId\":\"");
  Serial.print(sensorId);
  Serial.print("\",\"value\":");
  Serial.print(measurement, 2);
  Serial.print(",\"type\":\"");
  Serial.print(sensorType);
  Serial.println("\"}");
}
