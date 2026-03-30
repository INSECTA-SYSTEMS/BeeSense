# Bumblebee Detection v3 (Tracking + WiFi)

Version 3 ist die aktuellste Firmware-Variante mit Netzwerk-Anbindung.

## Funktionsumfang

- Kameraaufnahme und Bumblebee-Erkennung
- Linienbasierte Ein-/Ausflugzählung
- Speicherung annotierter Bilder auf SD-Karte
- periodischer HTTP-Upload an das Dashboard (`/api/tracking`)
- NTP-Zeitsynchronisierung

## Build & Flash

```powershell
cd hardware/firmware/bumblebee_detection/v3
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

## WiFi-Konfiguration

Vor dem Flashen SSID, Passwort und Dashboard-Host konfigurieren:

```powershell
idf.py menuconfig
```

Bereich: `BeeSense WiFi Configuration`

Details: `ESP32_SETUP.md`

## Erwarteter Datenfluss

ESP32 (`v3`) → `POST /api/tracking` auf Dashboard-Server → Live-Updates per SSE im Browser.