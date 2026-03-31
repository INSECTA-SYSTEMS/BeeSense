#include <Wire.h>
#include <Adafruit_HDC1000.h> // important to install this library

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Adafruit_HDC1000 hdc = Adafruit_HDC1000();

void setup() {
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  Serial.begin(115200);
  hdc.begin();
}

void loop() {
  Serial.print("temp: ");
  Serial.print(hdc.readTemperature());
  Serial.print("  hum: ");
  Serial.println(hdc.readHumidity());
  delay(100);

}