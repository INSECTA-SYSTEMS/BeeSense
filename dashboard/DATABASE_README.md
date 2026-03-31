# BeeSense Database Setup

## Overview

The BeeSense dashboard now uses SQLite for persistent data storage on your AWS server. All sensor readings and bee detections are automatically saved to the database.

## Database Schema

### Tables

#### `sensor_data`
Stores temperature and humidity readings.
```sql
- id: Auto-increment primary key
- temperature: Temperature in °C
- humidity: Humidity in %
- timestamp: Unix timestamp (milliseconds)
- created_at: Database insert time
```

#### `bee_detections`
Stores individual bee detection events.
```sql
- id: Auto-increment primary key
- direction: 'einflug' or 'ausflug'
- confidence: Detection confidence (0-1)
- image_path: Optional path to detection image
- timestamp: Unix timestamp (milliseconds)
- created_at: Database insert time
```

#### `daily_stats`
Aggregated daily statistics.
```sql
- id: Auto-increment primary key
- date: Date string (YYYY-MM-DD)
- einflug_count: Total entries for the day
- ausflug_count: Total exits for the day
- total_flights: Total flights (entries + exits)
- avg_temperature: Average temperature
- avg_humidity: Average humidity
- min_temperature: Minimum temperature
- max_temperature: Maximum temperature
- created_at: Database insert time
- updated_at: Last update time
```

#### `system_logs`
System logs for debugging.
```sql
- id: Auto-increment primary key
- level: 'info', 'warning', or 'error'
- message: Log message
- details: Additional details (JSON)
- timestamp: Unix timestamp (milliseconds)
- created_at: Database insert time
```

## Deployment to AWS

### Quick Deploy

From your `dashboard` directory on Windows:

```powershell
.\deploy-aws.ps1
```

This automatically:
1. Backs up existing database
2. Deploys updated code
3. Installs sqlite3 package
4. Restarts server

### Manual Steps

If you need to deploy manually:

```bash
# Connect to AWS
ssh ubuntu@3.75.94.127

# Stop server
pm2 stop beesense

# Backup existing database (if it exists)
cd ~/beesense
cp beesense.db beesense.db.backup

# Upload files (from your Windows machine in new terminal)
cd dashboard
scp server.js database.js package.json ubuntu@3.75.94.127:~/beesense/

# Back on AWS server
cd ~/beesense
npm install

# Start server
pm2 restart beesense
pm2 save
```

## Using the Database

### Automatic Data Storage

Data is automatically stored when received:
- **Sensor data**: Saved every time `/api/sensors` receives data
- **Bee detections**: Saved when `/api/tracking` receives einflug/ausflug counts
- **Daily stats**: Automatically calculated every 10 minutes

### Querying Data

#### Via API

```powershell
# Get last 7 days of historical data
curl http://3.75.94.127:8080/api/history?days=7

# Get last 30 days
curl http://3.75.94.127:8080/api/history?days=30
```

#### Direct Database Access

```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sqlite3 beesense.db

# Show all tables
.tables

# View daily statistics
SELECT * FROM daily_stats ORDER BY date DESC LIMIT 7;

# Count total detections
SELECT direction, COUNT(*) FROM bee_detections GROUP BY direction;

# View recent sensor readings
SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 10;

# Average temperature by day
SELECT DATE(created_at) as day, AVG(temperature) as avg_temp 
FROM sensor_data 
GROUP BY day 
ORDER BY day DESC 
LIMIT 7;

# Exit
.quit
```

## Database Maintenance

### Backups

**Automatic:** Created before each deployment as `beesense.db.backup`

**Manual backup:**
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
cp beesense.db beesense.db.backup.$(date +%Y%m%d)
```

### Restore from Backup

```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
pm2 stop beesense
cp beesense.db.backup beesense.db
pm2 restart beesense
```

### Download Database

To analyze data locally:
```powershell
scp ubuntu@3.75.94.127:~/beesense/beesense.db ./beesense.db

# Open with SQLite browser or query directly
sqlite3 beesense.db
```

### Clear Old Data

To keep database size manageable:
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sqlite3 beesense.db

# Delete detections older than 90 days
DELETE FROM bee_detections 
WHERE timestamp < (strftime('%s', 'now', '-90 days') * 1000);

# Delete sensor data older than 90 days
DELETE FROM sensor_data 
WHERE timestamp < (strftime('%s', 'now', '-90 days') * 1000);

# Vacuum to reclaim space
VACUUM;

.quit
```

## Troubleshooting

### Database not created

Check server logs:
```bash
pm2 logs beesense
```

Should see: `✅ Datenbank verbunden: /home/ubuntu/beesense/beesense.db`

### sqlite3 module error

Reinstall dependencies:
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
npm install sqlite3 --build-from-source
pm2 restart beesense
```

### Permissions error

Fix permissions:
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sudo chown ubuntu:ubuntu beesense.db
chmod 644 beesense.db
```

### Database locked

Stop all processes:
```bash
pm2 stop beesense
pm2 restart beesense
```

## Performance

- **Database size**: ~100KB per day with typical usage
- **Query speed**: <10ms for most queries
- **Concurrent connections**: Handled by SQLite WAL mode
- **Backups**: Minimal overhead, <1MB typically

## Future Enhancements

Possible upgrades:
- Automatic cleanup of old data
- Database compression
- Export to CSV/Excel
- Migration to AWS RDS for multi-server setups
- Grafana integration for advanced analytics
