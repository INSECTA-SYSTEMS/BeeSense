# ESP32 Configuration for AWS Dashboard

## Quick Setup

Your dashboard is running at: **http://3.75.94.127:8080**

### 1. Configure WiFi Credentials

You need to set your WiFi SSID and password so the ESP32 can connect to your network.

**Option A: Using menuconfig (Recommended)**
```bash
cd hardware/firmware/bumblebee_detection/v3
idf.py menuconfig
```

Navigate to: `BeeSense WiFi Configuration`
- Set **WiFi SSID** to your network name
- Set **WiFi Password** to your network password
- **Dashboard Server Host** is already set to `3.75.94.127`
- **Dashboard Server Port** is already set to `8080`

**Option B: Edit sdkconfig directly**

Add these lines to `sdkconfig` or `sdkconfig.defaults.esp32s3`:
```
CONFIG_WIFI_SSID="YOUR_WIFI_NAME"
CONFIG_WIFI_PASSWORD="YOUR_WIFI_PASSWORD"
CONFIG_DASHBOARD_HOST="3.75.94.127"
CONFIG_DASHBOARD_PORT=8080
```

### 2. Build and Flash

```bash
# Build the firmware
idf.py build

# Flash to ESP32
idf.py -p COM3 flash monitor
```

Replace `COM3` with your actual serial port (check Device Manager on Windows).

### 3. Monitor Output

After flashing, the ESP32 will:
1. Connect to WiFi
2. Start detecting bumblebees
3. Send tracking data to your AWS server every 10 seconds (or when data changes)

Expected output:
```
WiFi Station gestartet, verbinde...
IP-Adresse erhalten: 192.168.x.x
Sende Daten an http://3.75.94.127:8080/api/tracking
Daten erfolgreich gesendet
```

### 4. View Dashboard

Open your browser to: **http://3.75.94.127:8080**

You should see the bee flight data updating in real-time!

---

## Troubleshooting

### ESP32 won't connect to WiFi
- Double-check SSID and password in menuconfig
- Make sure ESP32 is in range of WiFi
- Try increasing `WIFI_MAXIMUM_RETRY` in menuconfig

### Data not appearing on dashboard
- Check ESP32 serial monitor for "Daten erfolgreich gesendet"
- Verify AWS security group allows port 8080
- Test server: `curl http://3.75.94.127:8080/api/tracking`
- Make sure PM2 is running on AWS: `pm2 list`

### Connection refused errors
- Restart the server: `pm2 restart beesense`
- Check if server is listening: `sudo netstat -tlnp | grep 8080`

---

## Configuration Summary

| Setting | Value |
|---------|-------|
| Dashboard URL | http://3.75.94.127:8080 |
| API Endpoint | http://3.75.94.127:8080/api/tracking |
| WiFi SSID | (Set by you in menuconfig) |
| WiFi Password | (Set by you in menuconfig) |
| Send Interval | 10 seconds |

---

## Data Format

The ESP32 sends JSON data like this:
```json
{
  "einflug": 5,
  "ausflug": 3,
  "timestamp": 1704900000000
}
```

- `einflug`: Number of bees entering the hive
- `ausflug`: Number of bees leaving the hive
- `timestamp`: Unix timestamp in milliseconds
