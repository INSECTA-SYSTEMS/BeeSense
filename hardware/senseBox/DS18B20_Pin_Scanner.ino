/*
 * DS18B20 Pin Scanner
 * 
 * Dieses Skript durchsucht alle digitalen Pins nach einem DS18B20 Sensor
 * und gibt aus, auf welchem Pin der Sensor gefunden wurde.
 * 
 * Anleitung:
 * 1. Lade dieses Skript auf dein Arduino/senseBox Board
 * 2. Öffne den seriellen Monitor (115200 Baud)
 * 3. Das Skript zeigt an, auf welchem Pin der DS18B20 gefunden wurde
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// Definiere den Bereich der zu testenden Pins
// Für Arduino Uno/senseBox MCU: Pins 0-13 (Pin 0 und 1 sind RX/TX, also vorsichtig)
// Für Arduino Mega: Pins 0-53
const int START_PIN = 2;  // Starte bei Pin 2 (0 und 1 sind oft für Serial reserviert)
const int END_PIN = 13;   // Bis Pin 13 testen

void setup() {
  Serial.begin(115200);
  
  // Warte einen Moment, damit der Serial Monitor bereit ist
  delay(2000);
  
  Serial.println("==========================================");
  Serial.println("  DS18B20 Pin Scanner");
  Serial.println("==========================================");
  Serial.println();
  Serial.print("Scanne Pins ");
  Serial.print(START_PIN);
  Serial.print(" bis ");
  Serial.print(END_PIN);
  Serial.println("...");
  Serial.println();
  
  bool sensorFound = false;
  
  // Durchlaufe alle Pins
  for (int pin = START_PIN; pin <= END_PIN; pin++) {
    Serial.print("Teste Pin ");
    Serial.print(pin);
    Serial.print("... ");
    
    // Erstelle OneWire-Instanz für diesen Pin
    OneWire oneWire(pin);
    DallasTemperature sensors(&oneWire);
    
    // Initialisiere den Sensor
    sensors.begin();
    
    // Prüfe, wie viele Geräte gefunden wurden
    int deviceCount = sensors.getDeviceCount();
    
    if (deviceCount > 0) {
      Serial.print("✓ GEFUNDEN! ");
      Serial.print(deviceCount);
      Serial.print(" Gerät(e) gefunden");
      Serial.println();
      
      // Zeige Details zu jedem gefundenen Gerät
      for (int i = 0; i < deviceCount; i++) {
        DeviceAddress deviceAddress;
        if (sensors.getAddress(deviceAddress, i)) {
          Serial.print("  → Gerät ");
          Serial.print(i);
          Serial.print(": ");
          printAddress(deviceAddress);
          Serial.println();
          
          // Teste, ob es wirklich ein DS18B20 ist
          Serial.print("     Typ: ");
          switch (deviceAddress[0]) {
            case 0x10:
              Serial.println("DS18S20 (alt)");
              break;
            case 0x28:
              Serial.println("DS18B20");
              break;
            case 0x22:
              Serial.println("DS1822");
              break;
            default:
              Serial.println("Unbekanntes Gerät");
          }
          
          // Teste eine Temperaturmessung
          sensors.requestTemperatures();
          float temp = sensors.getTempC(deviceAddress);
          Serial.print("     Temperatur: ");
          Serial.print(temp);
          Serial.println(" °C");
        }
      }
      
      sensorFound = true;
      Serial.println();
    } else {
      Serial.println("Nichts gefunden");
    }
    
    delay(100);  // Kurze Pause zwischen den Tests
  }
  
  Serial.println("==========================================");
  if (sensorFound) {
    Serial.println("Scan abgeschlossen! Sensor(en) gefunden.");
    Serial.println();
    Serial.println("NÄCHSTER SCHRITT:");
    Serial.println("Passe in deinem Code die Zeile:");
    Serial.println("  #define ONE_WIRE_BUS [PIN_NUMMER]");
    Serial.println("entsprechend an.");
  } else {
    Serial.println("KEIN DS18B20 SENSOR GEFUNDEN!");
    Serial.println();
    Serial.println("Mögliche Gründe:");
    Serial.println("- Sensor nicht angeschlossen");
    Serial.println("- Falsche Verkabelung (VCC, GND, Data)");
    Serial.println("- Pullup-Widerstand (4.7kΩ) fehlt");
    Serial.println("- Sensor defekt");
  }
  Serial.println("==========================================");
}

void loop() {
  // Nichts zu tun - Scanner läuft nur einmal in setup()
}

// Hilfsfunktion zum Ausgeben der Geräteadresse
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(":");
  }
}
