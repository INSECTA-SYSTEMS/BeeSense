/*
 * BeeSense – Nur DS18B20 Temperatur-Upload
 * ==========================================
 * Minimaler Sketch: Liest nur den DS18B20 aus und sendet
 * die Temperatur über Serial an die Bridge → OpenSenseMap.
 *
 * Kein I2C, keine anderen Sensoren → kein Bus-Konflikt.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 auf GPIO 2
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DeviceAddress ds18b20Address;

/* ------------------------------------------------------------------------- */
/* ---------------------------------Metadata-------------------------------- */
/* ------------------------------------------------------------------------- */
/* SENSEBOX ID  : 6994ab6bb3ae3d000839d743                                   */
/* SENSEBOX NAME: BeeSense                                                   */
/* ------------------------------------------------------------------------- */

#define SENSEBOX_ID "6994ab6bb3ae3d000839d743"

// Temperatur inside Beehive - DS18B20
#define SENSOR4_ID "6994ab6bb3ae3d000839d748"

// Uploadintervall in Millisekunden
const unsigned int postingInterval = 60000;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("INFO: BeeSense DS18B20-Only (ESP32-S3)");
  Serial.print("INFO: DS18B20 Pin: GPIO ");
  Serial.println(ONE_WIRE_BUS);

  pinMode(ONE_WIRE_BUS, INPUT);
  delay(100);

  oneWire.reset();
  delay(500);

  ds18b20.begin();
  ds18b20.setWaitForConversion(true);
  ds18b20.setResolution(12);

  int deviceCount = ds18b20.getDeviceCount();
  Serial.print("INFO: DS18B20 device count: ");
  Serial.println(deviceCount);

  if (deviceCount > 0 && ds18b20.getAddress(ds18b20Address, 0)) {
    Serial.print("INFO: DS18B20 Adresse: ");
    for (uint8_t i = 0; i < 8; i++) {
      if (ds18b20Address[i] < 16) Serial.print("0");
      Serial.print(ds18b20Address[i], HEX);
      if (i < 7) Serial.print(":");
    }
    Serial.println();

    // Aufwärm-Messung verwerfen
    ds18b20.requestTemperatures();
    delay(1000);
    float warmup = ds18b20.getTempC(ds18b20Address);
    Serial.print("INFO: Aufwaermmessung: ");
    Serial.print(warmup);
    Serial.println(" °C (verworfen)");
  } else {
    Serial.println("ERROR: DS18B20 nicht gefunden!");
  }

  Serial.println("INFO: Ready for data transmission");
}

void loop() {
  Serial.println("INFO: ===========================================");
  Serial.println("INFO: Reading DS18B20...");

  oneWire.reset();
  delay(50);

  ds18b20.requestTemperatures();
  delay(800);

  float temp = ds18b20.getTempC(ds18b20Address);

  Serial.print("INFO: Beehive Temperature (DS18B20): ");
  Serial.print(temp);
  Serial.println(" °C");

  if (temp > -50.0 && temp < 125.0 && temp != 85.0) {
    sendSensorData(temp, SENSOR4_ID, "Beehive_Temperature");
  } else {
    Serial.print("WARN: Ungueltiger Wert (");
    Serial.print(temp);
    Serial.println(" °C), wird NICHT gesendet");
  }

  Serial.println("INFO: ===========================================");
  Serial.print("INFO: Waiting ");
  Serial.print(postingInterval / 1000);
  Serial.println(" seconds...");
  Serial.println();

  delay(postingInterval);
}

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
