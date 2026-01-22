// Temperature, Humidity and UV Display

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_HDC1000.h>
#include <Adafruit_LTR329_LTR303.h> // Correct Adafruit library
#include <Adafruit_VEML6070.h>     // Correct Adafruit library

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_HDC1000 hdc = Adafruit_HDC1000();
Adafruit_LTR329 ltr = Adafruit_LTR329();
Adafruit_VEML6070 uv = Adafruit_VEML6070();

void displaySensorData() {
  display.clearDisplay();
  
  // Read sensor data
  float temp = hdc.readTemperature();
  float humi = hdc.readHumidity();
  
  // Read Light (LTR329)
  uint16_t visible_plus_ir, ir;
  bool lightValid = ltr.readBothChannels(visible_plus_ir, ir);
  unsigned int lightLevel = visible_plus_ir; 

  uint16_t uvLevel = uv.readUV();
  float uvIndex = uvLevel / 100.0;
  
  // Display title
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  display.println("BeeSense Monitor");
  
  // Draw separator line
  display.drawLine(0, 9, SCREEN_WIDTH, 9, WHITE);
  
  // Display temperature (left side)
  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("Temp:");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print(temp, 1);
  display.print("C");
  
  // Display humidity (right side)
  display.setTextSize(1);
  display.setCursor(70, 14);
  display.print("Hum:");
  display.setTextSize(2);
  display.setCursor(70, 24);
  display.print(humi, 0);
  display.print("%");
  
  // Draw separator line
  display.drawLine(0, 42, SCREEN_WIDTH, 42, WHITE);
  
  // Display Lux and UV
  display.setTextSize(1);
  display.setCursor(0, 46);
  display.print("Lux: ");
  display.print(lightLevel);

  // Display UV Index and level
  display.setCursor(0, 56);
  display.print("UV: ");
  display.print(uvLevel);
  display.print(" I:");
  display.print(uvIndex, 1);
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  
  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.setRotation(2);
  display.display();
  delay(100);
  display.clearDisplay();
  
  // Initialize HDC1000 sensor
  hdc.begin();
  
  // Initialize Light and UV sensors
  if (!ltr.begin()) {
    Serial.println("Couldn't find LTR sensor!");
  }
  
  uv.begin(VEML6070_1_T);
  
  // Show startup message
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("BeeSense");
  display.println("Initializing...");
  display.println("");
  display.println("Temp/Hum: OK");
  display.println("UV Sensor: OK");
  display.display();
  delay(2000);
}

void loop() {
  // Display sensor data
  displaySensorData();
  
  // Also print to Serial for debugging
  float temp = hdc.readTemperature();
  float humi = hdc.readHumidity();
  
  // Read Light (LTR329)
  uint16_t visible_plus_ir, ir;
  bool lightValid = ltr.readBothChannels(visible_plus_ir, ir);
  unsigned int lightLevel = visible_plus_ir; 

  uint16_t uvLevel = uv.readUV();
  float uvIndex = uvLevel / 100.0;
  
  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print("°C | Hum: ");
  Serial.print(humi, 1);
  Serial.print("% | Light: ");
  Serial.print(lightLevel);
  Serial.print(" | UV: ");
  Serial.print(uvLevel);
  Serial.print(" (Index: ");
  Serial.print(uvIndex, 1);
  Serial.println(")");
  
  // Update every 2 seconds
  delay(2000);
}