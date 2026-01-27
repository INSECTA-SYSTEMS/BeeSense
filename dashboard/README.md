# studyproject-bee-counting

# BeeSense Dashboard (PC Simulation)

Dieses Dashboard ist die **UI-Simulation** des BeeSense/Hummelstock-Projekts.  
Es dient dazu, die geplante Weboberfläche *ohne ESP32-Hardware* lokal im Browser zu entwickeln und zu testen.

Der Branch **`dashboard`** enthält ausschließlich den Web-Teil (HTML, CSS, JS) und ist **getrennt vom Firmware-Code** im `main`-Branch.

---

## Ziel des Dashboards

Das Dashboard soll später:

- die Live-Werte aus der ESP32-Firmware anzeigen  
- historische Daten (Flüge, Temperatur, Feuchtigkeit) visualisieren  
- Standortinformationen (GPS/Koordinaten) anzeigen  
- Warnungen und Statusinfos für den Imker geben  
- als Grundlage für die mobile/remote Weboberfläche dienen  

Aktuell läuft das Dashboard **mit simulierten Testdaten**, um UI-Design und Logik unabhängig von der Hardware entwickeln zu können.

---

## Aktuelle Funktionen

### Übersicht
- Live-Simulation von:
  - Flügen
  - Temperatur
  - Luftfeuchtigkeit
- Ampelstatus für Temperatur/Feuchtigkeit

### Flüge
- Tagesflüge
- Balkendiagramm der letzten 7 Tage (ohne heutigen Tag)

### Temperatur
- aktuelle Temperatur
- Liniendiagramm: Ø Temperatur der letzten 7 Tage (ohne heute)

### Standort
- Interaktive Karte (Leaflet)
- Eingabe von Koordinaten durch den Imker
- Speicherung im Browser (localStorage)
- Rechtskarte höhenverstellbar

### Navigation
- Reiterstruktur (Tabs) für saubere Übersicht

---

## Projektstruktur
dashboard/

│── index.html → Hauptdatei (UI, Charts, Karte, Simulation)

│── style.css → Farbschema, Layout, Kartenfarben

│── assets/ (optional)

│── README.md


Das Dashboard wird gerade noch nicht in das PlatformIO-Projekt eingebettet, sondern parallel getrennt entwickelt und später darauf gezogen.

---

## Zukunft: Anbindung an PlatformIO / ESP32 Firmware

Das Dashboard wird später:

1. **über einen Webserver auf dem ESP32 bereitgestellt**  
2. Daten über eine **REST-API oder WebSocket** vom ESP32 beziehen  
3. historische Daten aus dem Firmware-Speicher (Flash / SD-Karte) oder dem Backend laden  

### ✅ Implementiert (AWS Server: http://3.75.94.127:8080):

#### GET Endpoints (Daten abrufen)
- `/api/data` - Alle aktuellen Daten (Tracking + Sensoren)
- `/api/tracking` - Aktuelle Flugdaten (einflug/ausflug)
- `/api/sensors` - Aktuelle Sensordaten (Temperatur/Feuchtigkeit)
- `/api/history?days=7` - Historische Daten aus Datenbank

#### POST Endpoints (Daten senden)
- `/api/tracking` - Tracking-Daten vom ESP32 empfangen
- `/api/sensors` - Sensor-Daten empfangen (Temperatur/Feuchtigkeit)

### Beispiele für Sensor-Anbindung

#### ESP32/Arduino mit Sensor (z.B. DHT22, BME280):
```cpp
// HTTP POST request to send sensor data
String serverUrl = "http://3.75.94.127:8080/api/sensors";
String jsonData = "{\"temperature\":" + String(temp) + 
                  ",\"humidity\":" + String(humid) + 
                  ",\"timestamp\":" + String(millis()) + "}";

http.begin(serverUrl);
http.addHeader("Content-Type", "application/json");
int httpCode = http.POST(jsonData);
```

#### Python/Raspberry Pi mit Sensor:
```python
import requests
import time

# Send sensor data
data = {
    "temperature": 23.5,
    "humidity": 65.2,
    "timestamp": int(time.time() * 1000)
}
response = requests.post(
    "http://3.75.94.127:8080/api/sensors",
    json=data
)
```

#### PowerShell Test (Windows):
```powershell
$body = '{"temperature":23.5,"humidity":65.2,"timestamp":1704900000000}'
Invoke-RestMethod -Uri "http://3.75.94.127:8080/api/sensors" -Method Post -Body $body -ContentType "application/json"
```

---

## Dashboard lokal starten

Es werden **keine zusätzlichen Installationen** benötigt.

### Variante A – direkt per Browser öffnen

Einfach doppelklick auf:

index.html

Dann Browser öffnen:

http://localhost:5500


## Quellen
https://data.mendeley.com/datasets/8gb9r2yhfc/6