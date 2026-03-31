# BeeSense Database - Quick Start Guide

## ✅ What's Been Set Up

Your BeeSense dashboard now has **SQLite database support** for persistent data storage!

### Files Created/Modified:

1. **database.js** - Database module with all SQL operations
2. **database-config.js** - Configuration (supports SQLite + MySQL)
3. **server.js** - Updated to store all data in database
4. **package.json** - Added sqlite3 dependency
5. **deploy-aws.ps1** - Automated deployment script for Windows
6. **deploy-aws.sh** - Automated deployment script for Linux/Mac

## 🚀 Deploy to AWS

### Option 1: Automated (Recommended)

From your `dashboard` folder:

```powershell
.\deploy-aws.ps1
```

### Option 2: Manual

```bash
# Connect
ssh ubuntu@3.75.94.127

# Stop server
pm2 stop beesense

# Backup database
cd ~/beesense
cp beesense.db beesense.db.backup 2>/dev/null || true

# From Windows, upload files:
scp dashboard/server.js ubuntu@3.75.94.127:~/beesense/
scp dashboard/database.js ubuntu@3.75.94.127:~/beesense/
scp dashboard/package.json ubuntu@3.75.94.127:~/beesense/

# Back on AWS:
cd ~/beesense
npm install
pm2 restart beesense
pm2 save
```

## 📊 What Gets Stored

### Automatically Saved:
- ✅ **Sensor readings** (temperature & humidity) with timestamps
- ✅ **Bee detections** (entries & exits) with timestamps
- ✅ **Daily statistics** (calculated every 10 minutes)
- ✅ **System logs** for debugging

### Database Location:
`/home/ubuntu/beesense/beesense.db` on AWS

## 🔍 Query Your Data

### Via Browser/API:
```
http://3.75.94.127:8080/api/history?days=7
```

### Via SSH:
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
sqlite3 beesense.db

# View statistics
SELECT * FROM daily_stats ORDER BY date DESC LIMIT 7;

# Count detections
SELECT direction, COUNT(*) FROM bee_detections GROUP BY direction;

# Recent sensor data
SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 10;

.quit
```

## 📦 Database Tables

| Table | Purpose |
|-------|---------|
| `sensor_data` | Temperature & humidity readings |
| `bee_detections` | Individual bee entry/exit events |
| `daily_stats` | Aggregated daily statistics |
| `system_logs` | Server logs for debugging |

## 🔧 Maintenance

### Backup Database:
```bash
ssh ubuntu@3.75.94.127
cd ~/beesense
cp beesense.db beesense.db.backup.$(date +%Y%m%d)
```

### Download for Analysis:
```powershell
scp ubuntu@3.75.94.127:~/beesense/beesense.db ./beesense-local.db
```

### View Logs:
```bash
pm2 logs beesense
```

## 📖 Full Documentation

- **DATABASE_README.md** - Complete database documentation
- **AWS_DEPLOYMENT_GUIDE.md** - Full AWS deployment guide

## 💡 Next Steps

1. **Deploy to AWS**: Run `.\deploy-aws.ps1`
2. **Test it**: Send some data from ESP32 or use test commands
3. **Check data**: Query database or view `/api/history`
4. **Monitor**: Use `pm2 logs beesense` to see data being saved

## 🎯 Benefits

✅ **Persistent Storage** - Data survives server restarts  
✅ **Historical Analysis** - Query weeks/months of data  
✅ **Automatic Aggregation** - Daily stats calculated automatically  
✅ **Scalable** - Can migrate to AWS RDS if needed  
✅ **Simple** - No external database server required  

---

**Questions?** Check DATABASE_README.md for detailed information.
