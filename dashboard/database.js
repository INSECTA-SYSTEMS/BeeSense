/**
 * BeeSense Database Module
 * 
 * Manages SQLite database for storing sensor data and bee detections
 */

const sqlite3 = require('sqlite3').verbose();
const path = require('path');

class BeeSenseDatabase {
    constructor(dbPath = path.join(__dirname, 'beesense.db')) {
        this.db = new sqlite3.Database(dbPath, (err) => {
            if (err) {
                console.error('❌ Fehler beim Öffnen der Datenbank:', err.message);
            } else {
                console.log('✅ Datenbank verbunden:', dbPath);
                this.initializeTables();
            }
        });
    }

    /**
     * Initialize database tables
     */
    initializeTables() {
        this.db.serialize(() => {
            // Table: sensor_data (temperature & humidity)
            this.db.run(`
                CREATE TABLE IF NOT EXISTS sensor_data (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    temperature REAL,
                    humidity REAL,
                    timestamp INTEGER NOT NULL,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            `);

            // Table: bee_detections (individual bee detection events)
            this.db.run(`
                CREATE TABLE IF NOT EXISTS bee_detections (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    direction TEXT NOT NULL CHECK(direction IN ('einflug', 'ausflug')),
                    confidence REAL,
                    image_path TEXT,
                    timestamp INTEGER NOT NULL,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            `);

            // Table: daily_stats (aggregated daily statistics)
            this.db.run(`
                CREATE TABLE IF NOT EXISTS daily_stats (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    date TEXT NOT NULL UNIQUE,
                    einflug_count INTEGER DEFAULT 0,
                    ausflug_count INTEGER DEFAULT 0,
                    total_flights INTEGER DEFAULT 0,
                    avg_temperature REAL,
                    avg_humidity REAL,
                    min_temperature REAL,
                    max_temperature REAL,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            `);

            // Table: system_logs (for debugging and monitoring)
            this.db.run(`
                CREATE TABLE IF NOT EXISTS system_logs (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    level TEXT NOT NULL CHECK(level IN ('info', 'warning', 'error')),
                    message TEXT NOT NULL,
                    details TEXT,
                    timestamp INTEGER NOT NULL,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            `);

            // Create indexes for performance
            this.db.run(`CREATE INDEX IF NOT EXISTS idx_sensor_timestamp ON sensor_data(timestamp)`);
            this.db.run(`CREATE INDEX IF NOT EXISTS idx_detection_timestamp ON bee_detections(timestamp)`);
            this.db.run(`CREATE INDEX IF NOT EXISTS idx_daily_date ON daily_stats(date)`);
            this.db.run(`CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON system_logs(timestamp)`);

            console.log('✅ Datenbank-Tabellen initialisiert');
        });
    }

    /**
     * Insert sensor data
     */
    insertSensorData(temperature, humidity, timestamp = Date.now()) {
        return new Promise((resolve, reject) => {
            const stmt = this.db.prepare(`
                INSERT INTO sensor_data (temperature, humidity, timestamp)
                VALUES (?, ?, ?)
            `);
            
            stmt.run(temperature, humidity, timestamp, function(err) {
                if (err) {
                    reject(err);
                } else {
                    resolve(this.lastID);
                }
            });
            
            stmt.finalize();
        });
    }

    /**
     * Insert bee detection
     */
    insertBeeDetection(direction, confidence = null, imagePath = null, timestamp = Date.now()) {
        return new Promise((resolve, reject) => {
            const stmt = this.db.prepare(`
                INSERT INTO bee_detections (direction, confidence, image_path, timestamp)
                VALUES (?, ?, ?, ?)
            `);
            
            stmt.run(direction, confidence, imagePath, timestamp, function(err) {
                if (err) {
                    reject(err);
                } else {
                    resolve(this.lastID);
                }
            });
            
            stmt.finalize();
        });
    }

    /**
     * Get sensor data for a time range
     */
    getSensorData(startTime, endTime = Date.now(), limit = 1000) {
        return new Promise((resolve, reject) => {
            this.db.all(`
                SELECT * FROM sensor_data 
                WHERE timestamp BETWEEN ? AND ?
                ORDER BY timestamp DESC
                LIMIT ?
            `, [startTime, endTime, limit], (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    /**
     * Get bee detections for a time range
     */
    getBeeDetections(startTime, endTime = Date.now(), limit = 1000) {
        return new Promise((resolve, reject) => {
            this.db.all(`
                SELECT * FROM bee_detections 
                WHERE timestamp BETWEEN ? AND ?
                ORDER BY timestamp DESC
                LIMIT ?
            `, [startTime, endTime, limit], (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    /**
     * Get detection counts by direction for a time range
     */
    getDetectionCounts(startTime, endTime = Date.now()) {
        return new Promise((resolve, reject) => {
            this.db.all(`
                SELECT 
                    direction,
                    COUNT(*) as count,
                    AVG(confidence) as avg_confidence
                FROM bee_detections
                WHERE timestamp BETWEEN ? AND ?
                GROUP BY direction
            `, [startTime, endTime], (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    /**
     * Get latest sensor readings
     */
    getLatestSensorData() {
        return new Promise((resolve, reject) => {
            this.db.get(`
                SELECT * FROM sensor_data 
                ORDER BY timestamp DESC 
                LIMIT 1
            `, (err, row) => {
                if (err) reject(err);
                else resolve(row || null);
            });
        });
    }

    /**
     * Get daily statistics for a date range
     */
    getDailyStats(startDate, endDate) {
        return new Promise((resolve, reject) => {
            this.db.all(`
                SELECT * FROM daily_stats 
                WHERE date BETWEEN ? AND ?
                ORDER BY date ASC
            `, [startDate, endDate], (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    /**
     * Update or create daily statistics
     */
    updateDailyStats(date, stats) {
        return new Promise((resolve, reject) => {
            const stmt = this.db.prepare(`
                INSERT INTO daily_stats 
                (date, einflug_count, ausflug_count, total_flights, 
                 avg_temperature, avg_humidity, min_temperature, max_temperature, updated_at)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
                ON CONFLICT(date) DO UPDATE SET
                    einflug_count = excluded.einflug_count,
                    ausflug_count = excluded.ausflug_count,
                    total_flights = excluded.total_flights,
                    avg_temperature = excluded.avg_temperature,
                    avg_humidity = excluded.avg_humidity,
                    min_temperature = excluded.min_temperature,
                    max_temperature = excluded.max_temperature,
                    updated_at = CURRENT_TIMESTAMP
            `);
            
            stmt.run(
                date,
                stats.einflug_count || 0,
                stats.ausflug_count || 0,
                stats.total_flights || 0,
                stats.avg_temperature || null,
                stats.avg_humidity || null,
                stats.min_temperature || null,
                stats.max_temperature || null,
                function(err) {
                    if (err) {
                        reject(err);
                    } else {
                        resolve(this.lastID);
                    }
                }
            );
            
            stmt.finalize();
        });
    }

    /**
     * Calculate and update daily statistics from raw data
     */
    async calculateDailyStats(date) {
        const startOfDay = new Date(date);
        startOfDay.setHours(0, 0, 0, 0);
        const endOfDay = new Date(date);
        endOfDay.setHours(23, 59, 59, 999);

        const startTime = startOfDay.getTime();
        const endTime = endOfDay.getTime();

        // Get detection counts
        const detectionCounts = await this.getDetectionCounts(startTime, endTime);
        const einflug = detectionCounts.find(d => d.direction === 'einflug');
        const ausflug = detectionCounts.find(d => d.direction === 'ausflug');

        // Get sensor statistics
        const sensorStats = await new Promise((resolve, reject) => {
            this.db.get(`
                SELECT 
                    AVG(temperature) as avg_temp,
                    AVG(humidity) as avg_hum,
                    MIN(temperature) as min_temp,
                    MAX(temperature) as max_temp
                FROM sensor_data
                WHERE timestamp BETWEEN ? AND ?
            `, [startTime, endTime], (err, row) => {
                if (err) reject(err);
                else resolve(row);
            });
        });

        const stats = {
            einflug_count: einflug?.count || 0,
            ausflug_count: ausflug?.count || 0,
            total_flights: (einflug?.count || 0) + (ausflug?.count || 0),
            avg_temperature: sensorStats.avg_temp,
            avg_humidity: sensorStats.avg_hum,
            min_temperature: sensorStats.min_temp,
            max_temperature: sensorStats.max_temp
        };

        await this.updateDailyStats(date, stats);
        return stats;
    }

    /**
     * Insert system log
     */
    insertLog(level, message, details = null, timestamp = Date.now()) {
        return new Promise((resolve, reject) => {
            const stmt = this.db.prepare(`
                INSERT INTO system_logs (level, message, details, timestamp)
                VALUES (?, ?, ?, ?)
            `);
            
            stmt.run(level, message, details, timestamp, function(err) {
                if (err) {
                    reject(err);
                } else {
                    resolve(this.lastID);
                }
            });
            
            stmt.finalize();
        });
    }

    /**
     * Get system logs
     */
    getLogs(level = null, limit = 100) {
        return new Promise((resolve, reject) => {
            const query = level 
                ? `SELECT * FROM system_logs WHERE level = ? ORDER BY timestamp DESC LIMIT ?`
                : `SELECT * FROM system_logs ORDER BY timestamp DESC LIMIT ?`;
            
            const params = level ? [level, limit] : [limit];
            
            this.db.all(query, params, (err, rows) => {
                if (err) reject(err);
                else resolve(rows);
            });
        });
    }

    /**
     * Close database connection
     */
    close() {
        return new Promise((resolve, reject) => {
            this.db.close((err) => {
                if (err) reject(err);
                else {
                    console.log('🔒 Datenbank geschlossen');
                    resolve();
                }
            });
        });
    }
}

module.exports = BeeSenseDatabase;
