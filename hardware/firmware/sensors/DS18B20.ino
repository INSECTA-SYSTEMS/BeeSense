#include <OneWire.h>
#include <DallasTemperature.h>

// Sensor an Pin 2 (oft Teil des I2C/Qwiic Steckers auf manchen Boards, aber hier als GPIO genutzt)
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  while(!Serial) delay(10);
  Serial.println("DS18B20 Measurement Start");

  // Versuche internen Pullup zu aktivieren, falls kein externer Widerstand vorhanden ist
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

  sensors.begin();
}

void loop() {
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  if(tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Water Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");
  } else {
    Serial.println("Error: Could not read temperature data (-127). Check Connection!");
  }
  delay(1000);
}