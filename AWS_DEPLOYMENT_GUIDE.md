# BeeSense AWS Deployment Guide

## ✅ Completed Steps

### AWS Server Setup (DONE)
- ✅ Node.js installed on AWS Ubuntu instance
- ✅ Dashboard files uploaded to `/home/ubuntu/beesense`
- ✅ Server running on port 8080
- ✅ Security group configured (port 8080 open)
- ✅ SQLite database integrated for persistent storage

**Your Dashboard:** http://3.75.94.127:8080

---

## Deploying Database Updates

### Automated Deployment (Recommended)

**From your Windows machine:**
```powershell
cd dashboard
.\deploy-aws.ps1
```

This script will:
1. Stop the running server
2. Backup the existing database
3. Upload updated files
4. Install npm dependencies (including sqlite3)
5. Restart the server with PM2

### Manual Deployment

If the automated script doesn't work, deploy manually:

```powershell
# 1. Connect to AWS
ssh ubuntu@3.75.94.127

# 2. Stop server
pm2 stop beesense

# 3. Backup database (optional but recommended)
cd ~/beesense
cp beesense.db beesense.db.backup

# 4. Upload files from your local machine (in a new terminal)
scp dashboard/server.js ubuntu@3.75.94.127:~/beesense/
scp dashboard/database.js ubuntu@3.75.94.127:~/beesense/
scp dashboard/package.json ubuntu@3.75.94.127:~/beesense/

# 5. Back in SSH, install dependencies
cd ~/beesense
npm install

# 6. Restart server
pm2 restart beesense
pm2 save
```

---

## Database Features

### What's Stored

The SQLite database stores:
- **Sensor Data**: Temperature and humidity readings with timestamps
- **Bee Detections**: Individual bee entry/exit events
- **Daily Statistics**: Aggregated daily flight counts and sensor averages
- **System Logs**: Server logs for debugging

### Database File

**Location:** `/home/ubuntu/beesense/beesense.db`
**Type:** SQLite3
**Automatic Backups:** Created before each deployment

### Querying the Database

You can query the database directly on AWS:

```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sqlite3 beesense.db

# Example queries:
.tables                          # Show all tables
SELECT * FROM daily_stats;       # View daily statistics
SELECT * FROM bee_detections ORDER BY timestamp DESC LIMIT 10;
SELECT COUNT(*) FROM sensor_data;
.quit
```

### API Endpoints with Database

- `GET /api/history?days=7` - Get historical data (from database)
- `GET /api/tracking` - Current tracking data
- `GET /api/sensors` - Current sensor data

Example:
```powershell
curl http://3.75.94.127:8080/api/history?days=30
```

---

## Next Steps

### 1. Make Server Run Permanently

Currently, the server stops if you close the terminal. To keep it running 24/7:

**In your AWS SSH terminal:**
```bash
# Stop the current server (Ctrl+C), then:
sudo npm install -g pm2
cd ~/beesense
pm2 start server.js --name beesense
pm2 save
pm2 startup
```

Copy and run the command that PM2 shows you (it will look like `sudo env PATH=$PATH...`).

**Useful PM2 Commands:**
```bash
pm2 list              # Show running processes
pm2 logs beesense     # View logs
pm2 restart beesense  # Restart server
pm2 stop beesense     # Stop server
```

### 2. Configure ESP32

Your ESP32 v3 firmware already has WiFi support! You just need to configure it.

**Steps:**
1. Open terminal in: `hardware/firmware/bumblebee_detection/v3`
2. Run: `idf.py menuconfig`
3. Navigate to: `BeeSense WiFi Configuration`
4. Set:
   - **WiFi SSID**: Your WiFi network name
   - **WiFi Password**: Your WiFi password
   - **Dashboard Host**: Already set to `3.75.94.127` ✅
   - **Dashboard Port**: Already set to `8080` ✅
5. Save and exit (press Q, then Y)
6. Build: `idf.py build`
7. Flash: `idf.py -p COMX flash monitor` (replace COMX with your port)

**Full instructions:** See [ESP32_SETUP.md](hardware/firmware/bumblebee_detection/v3/ESP32_SETUP.md)

---

## System Architecture

```
┌─────────────┐          WiFi           ┌──────────────────┐
│   ESP32     │  ───────────────────>   │   AWS Server     │
│   Camera    │   HTTP POST JSON        │   3.75.94.127    │
│  (v3 FW)    │                          │   Port 8080      │
└─────────────┘                          └──────────────────┘
                                                  │
                                                  │ Server-Sent Events
                                                  │
                                          ┌───────▼──────────┐
                                          │   Web Browser    │
                                          │   Dashboard      │
                                          └──────────────────┘
```

### Data Flow
1. **ESP32** detects bees and counts entries/exits
2. **ESP32** sends JSON via HTTP POST every 10 seconds:
   ```json
   {
     "einflug": 5,
     "ausflug": 3,
     "timestamp": 1704900000000
   }
   ```
3. **AWS Server** receives data at `/api/tracking`
4. **Server** broadcasts to connected browsers via Server-Sent Events
5. **Dashboard** updates in real-time

---

## Testing the Setup

### Test 1: Check Server Status
```bash
# On AWS
pm2 status
```
Should show `beesense` with status `online`.

### Test 2: Test API Endpoint
From your Windows machine:
```powershell
curl http://3.75.94.127:8080/api/tracking
```
Should return current tracking data (initially zeros).

### Test 3: Test Dashboard
Open browser: http://3.75.94.127:8080
- Should see the BeeSense dashboard
- Values should be 0 initially
- Connection status should show "Warte auf Daten..."

### Test 4: Manual Data Test
Send test data to simulate ESP32:
```powershell
$body = '{"einflug":5,"ausflug":3,"timestamp":1704900000000}'
Invoke-RestMethod -Uri "http://3.75.94.127:8080/api/tracking" -Method Post -Body $body -ContentType "application/json"
```
Dashboard should update immediately!

---

## Configuration Files

### AWS Server
- **Location:** `/home/ubuntu/beesense/`
- **Main file:** `server.js`
- **Database:** `beesense.db` (SQLite)
- **Database module:** `database.js`
- **Port:** 8080
- **Process manager:** PM2

### ESP32 Firmware
- **Location:** `hardware/firmware/bumblebee_detection/v3/`
- **Config file:** `main/Kconfig.projbuild` (defaults updated)
- **WiFi manager:** `main/src/wifi_manager.cpp`
- **Main app:** `main/app_main.cpp`

---

## Troubleshooting

### Dashboard not accessible
1. Check AWS security group (port 8080 must be open)
2. Check server is running: `pm2 status`
3. Restart server: `pm2 restart beesense`

### ESP32 not connecting
1. Verify WiFi credentials in menuconfig
2. Check ESP32 is in WiFi range
3. View serial output: `idf.py monitor`
4. Look for "IP-Adresse erhalten" message

### Data not updating
1. Check ESP32 serial monitor for "Daten erfolgreich gesendet"
2. Check AWS server logs: `pm2 logs beesense`
3. Test API manually (see Test 4 above)

### Database issues
1. Check database file exists: `ssh ubuntu@3.75.94.127 "ls -lh ~/beesense/beesense.db"`
2. View recent logs: `pm2 logs beesense --lines 50`
3. Restore from backup if needed: `cp beesense.db.backup beesense.db`

### View database contents
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sqlite3 beesense.db "SELECT COUNT(*) as total_detections FROM bee_detections;"
sqlite3 beesense.db "SELECT * FROM daily_stats ORDER BY date DESC LIMIT 7;"
```

---

## Security Recommendations (Optional)

### 1. Restrict Access by IP
Edit AWS security group to allow port 8080 only from your IP address instead of `0.0.0.0/0`.

### 2. Add HTTPS
Install nginx and Let's Encrypt:
```bash
sudo apt install nginx certbot python3-certbot-nginx
sudo certbot --nginx -d your-domain.com
```

### 3. Add Authentication
Modify `server.js` to require API key or basic auth.

---

## Summary

| Component | Status | Location |
|-----------|--------|----------|
| AWS Server | ✅ Running | 3.75.94.127:8080 |
| Dashboard | ✅ Deployed | http://3.75.94.127:8080 |
| ESP32 Config | ⚠️ Needs WiFi setup | v3 firmware |
| PM2 Setup | ⏳ Recommended | Next step |

**Next Action:** Configure ESP32 WiFi credentials and flash the firmware!
