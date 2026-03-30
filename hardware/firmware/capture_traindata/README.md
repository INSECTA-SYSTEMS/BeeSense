# Capture Traindata Firmware

Diese Firmware erzeugt Trainingsdaten direkt auf dem ESP32-S3:

- Kameraaufnahme in jedem Zyklus
- Konvertierung und Crop auf `224x224`
- Speicherung als JPEG auf SD-Karte

Ziel ist ein kontinuierlicher Bildstrom für spätere Annotation und Modelltraining.

## Voraussetzungen

- ESP-IDF installiert
- ESP32-S3 Board mit Kamera + SD-Karte

## Build & Flash

```powershell
cd hardware/firmware/capture_traindata
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

## Laufzeitverhalten

- Aufnahmeintervall: ca. 1 Sekunde
- Bildgröße für Speicherung: `224x224`
- Ausgabeordner auf SD-Karte: `/sdcard/bumblebee_traindata`

## Troubleshooting

- Kamera init fail: Pin-Mapping und Versorgung prüfen
- SD mount fail: Karte formatieren (FAT32) und Kontakte prüfen
- Speicherprobleme: PSRAM aktivieren, Build-Config prüfen