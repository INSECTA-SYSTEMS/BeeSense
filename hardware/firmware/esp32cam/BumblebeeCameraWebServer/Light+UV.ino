#include <Wire.h>
#include <Adafruit_LTR329_LTR303.h>
#include <Adafruit_VEML6070.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

Adafruit_LTR329 ltr = Adafruit_LTR329();
Adafruit_VEML6070 uv = Adafruit_VEML6070();

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("Light & UV Sensor Test");

  // Initialize I2C with specific pins
  Wire.begin(PIN_QWIIC_SDA, PIN_QWIIC_SCL);

  // Initialize LTR329 (Light)
  if ( !ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
  } else {
    Serial.println("Found LTR sensor!");
    ltr.setGain(LTR3XX_GAIN_2);
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
    ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  }

  // Initialize VEML6070 (UV)
  uv.begin(VEML6070_1_T);
  Serial.println("VEML6070 UV sensor initialized");
}

void loop() {
  // Read Light (LTR329)
  uint16_t visible_plus_ir, ir;
  bool valid = ltr.readBothChannels(visible_plus_ir, ir);
  
  if (valid) {
    // Basic approximation of Lux. 
    // For accurate Lux, complex calculations involving gain/integration time/window factor are needed.
    // Ideally use visible_plus_ir - ir for visible light component.
    int visible = visible_plus_ir - ir; 
    if (visible < 0) visible = 0;
    
    Serial.print("Lux: "); Serial.print(visible);
  } else {
    Serial.print("Lux: Error");
  }

  // Read UV (VEML6070)
  uint16_t uvLevel = uv.readUV();
  Serial.print(" | UV: "); Serial.println(uvLevel);

  delay(1000);
}