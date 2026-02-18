-- Add sensor data (temperature & humidity)
INSERT INTO sensor_data (temperature, humidity, timestamp) VALUES 
(22.5, 65.3, strftime('%s','now') - 3600),
(23.1, 64.8, strftime('%s','now') - 3000),
(23.4, 63.5, strftime('%s','now') - 2400),
(24.0, 62.1, strftime('%s','now') - 1800),
(24.2, 61.5, strftime('%s','now') - 1200),
(23.8, 62.8, strftime('%s','now') - 600),
(23.5, 63.2, strftime('%s','now'));

-- Add bee detections
INSERT INTO bee_detections (direction, confidence, timestamp) VALUES 
('einflug', 0.92, strftime('%s','now') - 3500),
('einflug', 0.88, strftime('%s','now') - 3200),
('ausflug', 0.95, strftime('%s','now') - 2800),
('einflug', 0.91, strftime('%s','now') - 2500),
('ausflug', 0.87, strftime('%s','now') - 2100),
('einflug', 0.93, strftime('%s','now') - 1700),
('ausflug', 0.89, strftime('%s','now') - 1300),
('einflug', 0.94, strftime('%s','now') - 900),
('ausflug', 0.90, strftime('%s','now') - 500),
('einflug', 0.96, strftime('%s','now'));

-- Add daily stats (use INSERT OR REPLACE to handle existing dates)
INSERT OR REPLACE INTO daily_stats (date, einflug_count, ausflug_count, total_flights, avg_temperature, avg_humidity, min_temperature, max_temperature) VALUES 
(date('now'), 6, 4, 10, 23.5, 63.2, 22.5, 24.2),
(date('now', '-1 day'), 45, 42, 87, 21.3, 68.5, 18.2, 25.1),
(date('now', '-2 day'), 52, 48, 100, 22.8, 65.3, 19.5, 26.3);

-- Add system log
INSERT INTO system_logs (level, message, details, timestamp) VALUES 
('info', 'Test data inserted', 'Sample data added via SSH', strftime('%s','now'));
