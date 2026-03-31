# BeeSense

BeeSense ist ein Projekt zur automatisierten Erfassung von Hummel-/Bienenflügen.
Das Repository kombiniert Datensätze, Modelltraining, ESP32-Firmware, Dashboard und AWS-Deployment.

## Live-Dashboard

Das Dashboard ist online erreichbar unter:

`http://3.75.94.127:8080`

## Was dieses Repository enthält

- `dashboard/` – Web-Dashboard + Node.js-Server + SQLite-Datenhaltung
- `models/` – YOLO-Training, ONNX-Export, Kalibrierung, ESP-Quantisierung
- `hardware/firmware/` – ESP32-S3 Firmware für Erkennung und Datensammlung
- `hardware/firmware/sensors/` – Arduino-Sketches für Sensoren
- `data/` – Datensätze im YOLO-Format (`train/`, `val/`, `test/`)

## Schnellstart

### 1) Dashboard lokal starten

```powershell
cd dashboard
npm install
npm start
```

Dann im Browser öffnen: `http://localhost:8085`

### 2) Modelltraining starten

```powershell
cd models
python train.py
```

### 3) ESP32-Firmware bauen (Beispiel v3)

```powershell
cd hardware/firmware/bumblebee_detection/v3
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

## Wichtige Dokumentation

- `dashboard/README.md` – Dashboard & API Nutzung
- `dashboard/DATABASE_README.md` – Datenbankschema & Betrieb
- `models/README.md` – Trainings- und Exportpipeline
- `hardware/firmware/bumblebee_detection/v3/README.md` – aktuelle Erkennungs-Firmware
- `AWS_DEPLOYMENT_GUIDE.md` – Deployment auf EC2
