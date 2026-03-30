/**
 * BeeSense Dashboard Server
 * 
 * Empfängt Tracking-Daten vom ESP32 und pusht sie live ans Dashboard.
 * 
 * Starten mit: node server.js
 * Dashboard öffnen: http://localhost:8085
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { createDatabase } = require('./database-config');

const PORT = 8085;

// Initialize database
const db = createDatabase();

// Aktuelle Tracking-Daten
let trackingData = {
    einflug: 0,
    ausflug: 0,
    timestamp: Date.now(),
    lastUpdate: null
};

// Aktuelle Sensor-Daten (Temperatur & Feuchtigkeit)
let sensorData = {
    temperature: null,
    humidity: null,
    waterTemperature: null,
    lux: null,
    uv: null,
    timestamp: Date.now(),
    lastUpdate: null
};

// Load today's stats from database on startup
async function loadTodayStats() {
    try {
        const today = new Date().toISOString().split('T')[0];
        const stats = await db.getDailyStats(today, today);
        if (stats && stats.length > 0) {
            trackingData.einflug = stats[0].einflug_count || 0;
            trackingData.ausflug = stats[0].ausflug_count || 0;
            trackingData.lastUpdate = new Date().toISOString();
            console.log(`📊 Heute geladen: ${trackingData.einflug} Einflüge, ${trackingData.ausflug} Ausflüge`);
        }

        // Also load latest sensor data
        const latestSensor = await db.getLatestSensorData();
        if (latestSensor) {
            sensorData.temperature = latestSensor.temperature;
            sensorData.humidity = latestSensor.humidity;
            sensorData.waterTemperature = latestSensor.water_temperature ?? null;
            sensorData.lux = latestSensor.lux ?? null;
            sensorData.uv = latestSensor.uv ?? null;
            sensorData.lastUpdate = new Date().toISOString();
            console.log(`🌡️ Sensoren geladen: ${sensorData.temperature}°C, ${sensorData.humidity}%`);
        }
    } catch (err) {
        console.error('Fehler beim Laden der Tagesdaten:', err.message);
    }
}

// Server-Sent Events Clients (für Live-Updates)
const sseClients = new Set();

// MIME Types für statische Dateien
const mimeTypes = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'application/javascript',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.svg': 'image/svg+xml',
    '.json': 'application/json'
};

// HTTP Server
const server = http.createServer((req, res) => {
    // CORS Headers für lokale Entwicklung
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    // OPTIONS Request (CORS Preflight)
    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    const url = new URL(req.url, `http://${req.headers.host || `localhost:${PORT}`}`);
    
    // API: Tracking-Daten empfangen (POST vom ESP32)
    if (url.pathname === '/api/tracking' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                
                // Daten aktualisieren
                trackingData = {
                    einflug: data.einflug || 0,
                    ausflug: data.ausflug || 0,
                    timestamp: data.timestamp || Date.now(),
                    lastUpdate: new Date().toISOString()
                };

                console.log(`[${new Date().toLocaleTimeString()}] Tracking empfangen:`, trackingData);

                // In Datenbank speichern
                if (data.einflug > 0) {
                    db.insertBeeDetection('einflug', null, null, trackingData.timestamp)
                        .catch(err => console.error('DB Error (einflug):', err.message));
                }
                if (data.ausflug > 0) {
                    db.insertBeeDetection('ausflug', null, null, trackingData.timestamp)
                        .catch(err => console.error('DB Error (ausflug):', err.message));
                }

                // Alle SSE Clients benachrichtigen
                broadcastToClients(trackingData);

                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, message: 'Daten empfangen' }));
            } catch (e) {
                console.error('Fehler beim Parsen:', e.message);
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, error: 'Invalid JSON' }));
            }
        });
        return;
    }

    // API: Sensor-Daten empfangen (POST vom Arduino/Sensor)
    if (url.pathname === '/api/sensors' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                
                // Daten aktualisieren
                sensorData = {
                    temperature: data.temperature ?? sensorData.temperature,
                    humidity: data.humidity ?? sensorData.humidity,
                    waterTemperature: data.waterTemperature ?? sensorData.waterTemperature,
                    lux: data.lux ?? sensorData.lux,
                    uv: data.uv ?? sensorData.uv,
                    timestamp: data.timestamp || Date.now(),
                    lastUpdate: new Date().toISOString()
                };

                console.log(`[${new Date().toLocaleTimeString()}] Sensor-Daten empfangen:`, sensorData);

                // In Datenbank speichern
                db.insertSensorData(
                    sensorData.temperature, sensorData.humidity,
                    sensorData.waterTemperature, sensorData.lux, sensorData.uv,
                    sensorData.timestamp
                ).catch(err => console.error('DB Error (sensor):', err.message));

                // Alle SSE Clients benachrichtigen
                broadcastToClients({ tracking: trackingData, sensors: sensorData });

                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, message: 'Sensor-Daten empfangen' }));
            } catch (e) {
                console.error('Fehler beim Parsen:', e.message);
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, error: 'Invalid JSON' }));
            }
        });
        return;
    }

    // API: Wassertemperatur empfangen (POST vom DS18B20-Sensor)
    if (url.pathname === '/api/sensors/water' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                sensorData.waterTemperature = data.waterTemperature ?? data.temperature ?? sensorData.waterTemperature;
                sensorData.timestamp = data.timestamp || Date.now();
                sensorData.lastUpdate = new Date().toISOString();

                console.log(`[${new Date().toLocaleTimeString()}] Wassertemperatur empfangen: ${sensorData.waterTemperature}°C`);

                db.insertSensorData(
                    sensorData.temperature, sensorData.humidity,
                    sensorData.waterTemperature, sensorData.lux, sensorData.uv,
                    sensorData.timestamp
                ).catch(err => console.error('DB Error (water sensor):', err.message));

                broadcastToClients({ tracking: trackingData, sensors: sensorData });

                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, message: 'Wassertemperatur empfangen' }));
            } catch (e) {
                console.error('Fehler beim Parsen:', e.message);
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, error: 'Invalid JSON' }));
            }
        });
        return;
    }

    // API: Wassertemperatur abrufen (GET)
    if (url.pathname === '/api/sensors/water' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            waterTemperature: sensorData.waterTemperature,
            timestamp: sensorData.timestamp,
            lastUpdate: sensorData.lastUpdate
        }));
        return;
    }

    // API: Licht- & UV-Daten empfangen (POST vom LTR329/VEML6070-Sensor)
    if (url.pathname === '/api/sensors/light' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                sensorData.lux = data.lux ?? sensorData.lux;
                sensorData.uv = data.uv ?? sensorData.uv;
                sensorData.timestamp = data.timestamp || Date.now();
                sensorData.lastUpdate = new Date().toISOString();

                console.log(`[${new Date().toLocaleTimeString()}] Licht/UV empfangen: ${sensorData.lux} Lux, UV: ${sensorData.uv}`);

                db.insertSensorData(
                    sensorData.temperature, sensorData.humidity,
                    sensorData.waterTemperature, sensorData.lux, sensorData.uv,
                    sensorData.timestamp
                ).catch(err => console.error('DB Error (light sensor):', err.message));

                broadcastToClients({ tracking: trackingData, sensors: sensorData });

                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, message: 'Licht/UV-Daten empfangen' }));
            } catch (e) {
                console.error('Fehler beim Parsen:', e.message);
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: false, error: 'Invalid JSON' }));
            }
        });
        return;
    }

    // API: Licht- & UV-Daten abrufen (GET)
    if (url.pathname === '/api/sensors/light' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({
            lux: sensorData.lux,
            uv: sensorData.uv,
            timestamp: sensorData.timestamp,
            lastUpdate: sensorData.lastUpdate
        }));
        return;
    }

    // API: Aktuelle Daten abrufen (GET)
    if (url.pathname === '/api/tracking' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(trackingData));
        return;
    }

    // API: Sensor-Daten abrufen (GET)
    if (url.pathname === '/api/sensors' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(sensorData));
        return;
    }

    // API: Alle Daten abrufen (GET)
    if (url.pathname === '/api/data' && req.method === 'GET') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ tracking: trackingData, sensors: sensorData }));
        return;
    }

    // API: Historische Daten abrufen (GET)
    if (url.pathname === '/api/history' && req.method === 'GET') {
        // Get date range from query parameters (default: last 7 days)
        const days = parseInt(url.searchParams.get('days')) || 7;
        const endDate = new Date().toISOString().split('T')[0];
        const startDate = new Date(Date.now() - days * 24 * 60 * 60 * 1000).toISOString().split('T')[0];
        
        db.getDailyStats(startDate, endDate)
            .then(stats => {
                // Convert to format expected by frontend
                const formattedData = {
                    flights: stats.map(s => ({
                        date: s.date,
                        einflug: s.einflug_count,
                        ausflug: s.ausflug_count,
                        total: s.total_flights
                    })),
                    temperature: stats.map(s => ({
                        date: s.date,
                        avg: s.avg_temperature,
                        values: [s.min_temperature, s.avg_temperature, s.max_temperature].filter(v => v !== null)
                    })),
                    humidity: stats.map(s => ({
                        date: s.date,
                        avg: s.avg_humidity,
                        values: [s.avg_humidity].filter(v => v !== null)
                    }))
                };
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify(formattedData));
            })
            .catch(err => {
                console.error('DB Error (history):', err.message);
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ flights: [], temperature: [], humidity: [] }));
            });
        return;
    }

    // API: Hourly history data (for heatmap & hourly chart)
    if (url.pathname === '/api/history/hourly' && req.method === 'GET') {
        const days = parseInt(url.searchParams.get('days')) || 7;
        const startTime = Date.now() - days * 24 * 60 * 60 * 1000;

        const query = `
            SELECT 
                strftime('%Y-%m-%d', datetime(timestamp/1000, 'unixepoch', 'localtime')) as date,
                CAST(strftime('%H', datetime(timestamp/1000, 'unixepoch', 'localtime')) AS INTEGER) as hour,
                COUNT(*) as count,
                SUM(CASE WHEN direction = 'einflug' THEN 1 ELSE 0 END) as einflug,
                SUM(CASE WHEN direction = 'ausflug' THEN 1 ELSE 0 END) as ausflug
            FROM bee_detections
            WHERE timestamp >= ?
            GROUP BY date, hour
            ORDER BY date ASC, hour ASC
        `;

        try {
            db.db.all(query, [startTime], (err, rows) => {
                if (err) {
                    console.error('DB Error (hourly):', err.message);
                    res.writeHead(200, { 'Content-Type': 'application/json' });
                    res.end(JSON.stringify([]));
                    return;
                }
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify(rows || []));
            });
        } catch (e) {
            console.error('Hourly API error:', e.message);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify([]));
        }
        return;
    }

    // API: Server-Sent Events für Live-Updates
    if (url.pathname === '/api/events') {
        res.writeHead(200, {
            'Content-Type': 'text/event-stream',
            'Cache-Control': 'no-cache',
            'Connection': 'keep-alive'
        });

        // Initiale Daten senden
        const initialData = { tracking: trackingData, sensors: sensorData };
        res.write(`data: ${JSON.stringify(initialData)}\n\n`);

        // Client zur Liste hinzufügen
        sseClients.add(res);
        console.log(`[SSE] Client verbunden. Aktive Clients: ${sseClients.size}`);

        // Cleanup bei Disconnect
        req.on('close', () => {
            sseClients.delete(res);
            console.log(`[SSE] Client getrennt. Aktive Clients: ${sseClients.size}`);
        });
        return;
    }

    // Statische Dateien servieren
    let filePath = url.pathname;
    if (filePath === '/') filePath = '/index.html';
    
    const fullPath = path.join(__dirname, filePath);
    const ext = path.extname(fullPath);
    const contentType = mimeTypes[ext] || 'application/octet-stream';

    fs.readFile(fullPath, (err, data) => {
        if (err) {
            if (err.code === 'ENOENT') {
                res.writeHead(404);
                res.end('Datei nicht gefunden: ' + filePath);
            } else {
                res.writeHead(500);
                res.end('Server Fehler');
            }
            return;
        }

        // Add cache-control headers to prevent caching
        res.writeHead(200, { 
            'Content-Type': contentType,
            'Cache-Control': 'no-cache, no-store, must-revalidate',
            'Pragma': 'no-cache',
            'Expires': '0'
        });
        res.end(data);
    });
});

// Broadcast an alle SSE Clients
function broadcastToClients(data) {
    const message = `data: ${JSON.stringify(data)}\n\n`;
    sseClients.forEach(client => {
        try {
            client.write(message);
        } catch (e) {
            sseClients.delete(client);
        }
    });
}

// Aggregiere tägliche Daten
async function aggregateDailyData() {
    const today = new Date().toISOString().split('T')[0];
    
    try {
        // Calculate stats from database
        await db.calculateDailyStats(today);
        console.log(`[${new Date().toLocaleTimeString()}] Tägliche Daten in DB aggregiert`);
    } catch (err) {
        console.error('Fehler bei Aggregierung:', err.message);
    }
}

// Aggregiere Daten alle 10 Minuten
setInterval(aggregateDailyData, 10 * 60 * 1000);

// Start server only after loading today's data from DB
async function startServer() {
    // Wait briefly for DB tables to initialize
    await new Promise(resolve => setTimeout(resolve, 300));
    // Load today's data before accepting connections
    await aggregateDailyData();
    await loadTodayStats();

    server.listen(PORT, '0.0.0.0', () => {
        console.log('');
        console.log('🐝 BeeSense Dashboard Server gestartet');
        console.log('=====================================');
        console.log(`📊 Dashboard:        http://localhost:${PORT}`);
        console.log(`📡 Tracking API:     http://localhost:${PORT}/api/tracking`);
        console.log(`🌡️  Sensor API:       http://localhost:${PORT}/api/sensors`);
        console.log(`� Water Temp API:   http://localhost:${PORT}/api/sensors/water`);
        console.log(`💡 Light/UV API:     http://localhost:${PORT}/api/sensors/light`);
        console.log(`�📋 All Data API:     http://localhost:${PORT}/api/data`);
        console.log(`🔴 Live Events:      http://localhost:${PORT}/api/events`);
        console.log('');
        console.log('Warte auf Daten vom ESP32 und Sensoren...');
        console.log('');
    });
}

startServer();
