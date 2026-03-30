# BeeSense Sensor Connection Guide

Complete guide for connecting temperature and humidity sensors to the BeeSense dashboard.

## 📊 Server Information

**Dashboard URL:** http://3.75.94.127:8080  
**Sensor API Endpoint:** `POST /api/sensors`  
**Database:** SQLite (auto-switches to MySQL with environment variables)

---

## 📡 Supported Data Format

### Required JSON Structure
```json
{
  "temperature": 23.5,
  "humidity": 65.2,
  "timestamp": 1704900000000
}
```

### Field Descriptions
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `temperature` | Number | Yes | Temperature in °C |
| `humidity` | Number | Yes | Relative humidity in % |
| `timestamp` | Integer | Optional | Unix timestamp in milliseconds (defaults to server time) |

---

## 🔌 Connection Methods

### 1. ESP32 / ESP8266 (Arduino Framework)

#### Hardware Setup
- **Sensor:** DHT22, DHT11, BME280, or similar
- **Connection:** I2C or Digital Pin
- **WiFi:** Built-in

#### Code Example (ESP32 + DHT22)
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server configuration
const char* serverUrl = "http://3.75.94.127:8080/api/sensors";

// DHT22 setup
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (!isnan(temperature) && !isnan(humidity)) {
    sendSensorData(temperature, humidity);
  }
  
  delay(10000); // Send every 10 seconds
}

void sendSensorData(float temp, float humid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    String jsonData = "{\"temperature\":" + String(temp) + 
                      ",\"humidity\":" + String(humid) + 
                      ",\"timestamp\":" + String(millis()) + "}";
    
    int httpCode = http.POST(jsonData);
    
    if (httpCode == 200) {
      Serial.println("✅ Data sent successfully");
    } else {
      Serial.println("❌ Error: " + String(httpCode));
    }
    
    http.end();
  }
}
```

#### Required Libraries
```
DHT sensor library (Adafruit)
WiFi (built-in)
HTTPClient (built-in)
```

---

### 2. Arduino with WiFi Shield

#### Hardware Requirements
- Arduino Uno/Mega
- ESP8266 WiFi module or Arduino WiFi Shield
- DHT22 or BME280 sensor

#### Code Example
```cpp
#include <WiFi.h>
#include <DHT.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* server = "3.75.94.127";
const int port = 8080;

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  
  if (!isnan(temp) && !isnan(humid)) {
    sendData(temp, humid);
  }
  
  delay(10000);
}

void sendData(float temp, float humid) {
  if (client.connect(server, port)) {
    String json = "{\"temperature\":" + String(temp) + 
                  ",\"humidity\":" + String(humid) + "}";
    
    client.println("POST /api/sensors HTTP/1.1");
    client.println("Host: 3.75.94.127");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println();
    client.println(json);
    client.stop();
  }
}
```

---

### 3. Raspberry Pi (Python)

#### Installation
```bash
sudo apt-get update
sudo apt-get install python3-pip
pip3 install requests adafruit-dht
```

#### Code Example (DHT22 on GPIO4)
```python
#!/usr/bin/env python3
import time
import requests
import Adafruit_DHT

# Sensor configuration
DHT_SENSOR = Adafruit_DHT.DHT22
DHT_PIN = 4

# Server configuration
SERVER_URL = "http://3.75.94.127:8080/api/sensors"

def read_and_send_data():
    humidity, temperature = Adafruit_DHT.read_retry(DHT_SENSOR, DHT_PIN)
    
    if humidity is not None and temperature is not None:
        data = {
            "temperature": round(temperature, 1),
            "humidity": round(humidity, 1),
            "timestamp": int(time.time() * 1000)
        }
        
        try:
            response = requests.post(SERVER_URL, json=data, timeout=5)
            if response.status_code == 200:
                print(f"✅ Sent: {temperature}°C, {humidity}%")
            else:
                print(f"❌ Error: {response.status_code}")
        except Exception as e:
            print(f"❌ Connection error: {e}")
    else:
        print("❌ Failed to read sensor")

if __name__ == "__main__":
    while True:
        read_and_send_data()
        time.sleep(10)  # Send every 10 seconds
```

#### Run as Service
Create `/etc/systemd/system/beesense-sensor.service`:
```ini
[Unit]
Description=BeeSense Sensor Service
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi
ExecStart=/usr/bin/python3 /home/pi/sensor.py
Restart=always

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable beesense-sensor
sudo systemctl start beesense-sensor
sudo systemctl status beesense-sensor
```

---

### 4. Node.js (Linux/Mac/Windows)

#### Installation
```bash
npm install node-dht-sensor axios
```

#### Code Example
```javascript
const sensor = require('node-dht-sensor');
const axios = require('axios');

const SERVER_URL = 'http://3.75.94.127:8080/api/sensors';
const DHT_PIN = 4;
const DHT_TYPE = 22; // DHT22

async function readAndSend() {
    sensor.read(DHT_TYPE, DHT_PIN, async (err, temperature, humidity) => {
        if (!err) {
            const data = {
                temperature: Math.round(temperature * 10) / 10,
                humidity: Math.round(humidity * 10) / 10,
                timestamp: Date.now()
            };
            
            try {
                await axios.post(SERVER_URL, data);
                console.log(`✅ Sent: ${temperature}°C, ${humidity}%`);
            } catch (error) {
                console.error('❌ Error:', error.message);
            }
        }
    });
}

// Send every 10 seconds
setInterval(readAndSend, 10000);
```

---

## 🧪 Testing

### Test 1: Manual Test (PowerShell/Windows)
```powershell
$body = '{"temperature":23.5,"humidity":65.2}'
Invoke-RestMethod -Uri "http://3.75.94.127:8080/api/sensors" -Method Post -Body $body -ContentType "application/json"
```

### Test 2: Manual Test (curl/Linux/Mac)
```bash
curl -X POST http://3.75.94.127:8080/api/sensors \
  -H "Content-Type: application/json" \
  -d '{"temperature":23.5,"humidity":65.2}'
```

### Test 3: Verify Data on Dashboard
1. Open: http://3.75.94.127:8080
2. Check temperature and humidity values update
3. View historical data in charts

### Test 4: Check Database (SSH)
```bash
ssh -i "path/to/setup.pem" ubuntu@3.75.94.127
cd ~/beesense
pm2 logs beesense --lines 20
```

Look for: `Sensor-Daten empfangen: { temperature: 23.5, humidity: 65.2 }`

---

## 📊 Database Storage

Sensor data is automatically stored in the SQLite database:

**Table:** `sensor_data`  
**Columns:**
- `id` - Auto-increment primary key
- `temperature` - Temperature in °C
- `humidity` - Relative humidity in %
- `timestamp` - Unix timestamp (milliseconds)
- `created_at` - Database insertion time

**Indexes:** Timestamp index for fast queries

---

## 🔧 Troubleshooting

### Sensor Not Sending Data

1. **Check WiFi Connection**
   ```cpp
   Serial.println(WiFi.localIP()); // Should show IP address
   ```

2. **Verify Server Reachability**
   ```bash
   ping 3.75.94.127
   curl http://3.75.94.127:8080/api/data
   ```

3. **Check HTTP Response Code**
   - `200` = Success
   - `400` = Invalid JSON format
   - `500` = Server error

4. **Validate JSON Format**
   Use online JSON validator before sending

### Data Not Appearing on Dashboard

1. **Check PM2 Logs**
   ```bash
   ssh ubuntu@3.75.94.127 "pm2 logs beesense --lines 50"
   ```

2. **Verify Database**
   ```bash
   # Recent sensor readings
   curl http://3.75.94.127:8080/api/sensors
   ```

3. **Check Browser Console**
   - Open browser DevTools (F12)
   - Look for JavaScript errors
   - Check Network tab for API calls

### Sensor Reading Errors

1. **DHT Sensor Returns NaN**
   - Check wiring connections
   - Add pull-up resistor (10kΩ)
   - Increase delay between reads
   - Try different GPIO pin

2. **BME280 Not Found**
   - Verify I2C address (`0x76` or `0x77`)
   - Check I2C wiring (SDA, SCL)
   - Test with I2C scanner

---

## 🔐 Security Considerations

### For Production Use:

1. **Use HTTPS**
   ```
   Replace http:// with https:// after SSL setup
   ```

2. **Add Authentication**
   ```cpp
   http.addHeader("Authorization", "Bearer YOUR_API_KEY");
   ```

3. **Restrict IP Access**
   Configure AWS Security Group to allow only your sensor IPs

---

## 📈 Best Practices

1. **Send Interval:** 10-60 seconds (avoid overloading server)
2. **Error Handling:** Retry on failure with exponential backoff
3. **Data Validation:** Check sensor readings before sending
4. **Battery Optimization:** Use deep sleep for battery-powered sensors
5. **Timestamp:** Always include timestamp for accurate historical data

---

## 📚 Additional Resources

- [Dashboard README](README.md)
- [AWS Deployment Guide](../AWS_DEPLOYMENT_GUIDE.md)
- [Database Documentation](DATABASE_README.md)
- [ESP32 Setup Guide](../hardware/firmware/bumblebee_detection/v3/ESP32_SETUP.md)

---

## 💡 Example Sensors

| Sensor | Temperature | Humidity | Interface | Price |
|--------|-------------|----------|-----------|-------|
| DHT22 | ±0.5°C | ±2% | Digital | €5-10 |
| DHT11 | ±2°C | ±5% | Digital | €2-5 |
| BME280 | ±1°C | ±3% | I2C/SPI | €5-15 |
| SHT31 | ±0.3°C | ±2% | I2C | €10-20 |

**Recommendation:** DHT22 for budget builds, BME280 for accuracy, SHT31 for professional use

---

**Questions?** Check the dashboard logs or open an issue!
