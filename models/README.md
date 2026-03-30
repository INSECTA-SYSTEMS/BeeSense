# Models

Dieser Ordner enthält die komplette Trainings- und Deployment-Pipeline für das Bumblebee-Detektionsmodell.

## Inhalte

- `train.py` – YOLO-Training
- `predict_pictures.py` – Inferenz auf Testbildern
- `export_onnx.py` – ONNX-Export (ESP-kompatibel)
- `prepare_calib_data.py` – Kalibrierungsbilder erzeugen
- `quantize_onnx_model.py` – Quantisierung zu `.espdl`
- `yolov11_bumblebee.yaml` – Datensatz-Konfiguration

## Voraussetzungen

- Python 3.10+
- Pakete: `ultralytics`, `torch`, `onnx`, `onnxsim`, `Pillow`, `torchvision`, `esp-ppq`

## 1) Training

```powershell
cd models
python train.py
```

Ergebnis: `runs/detect/train_224_224/weights/best.pt`

## 2) Testen / Inferenz

```powershell
cd models
python predict_pictures.py
```

Standardquelle: `../data/bumblebees/images/test/`  
Ausgabe: `runs/detect/predict/`

## 3) Export und Quantisierung für ESP32-S3

```powershell
cd models

# ONNX exportieren
python export_onnx.py

# Kalibrierdaten erzeugen (32 Bilder, 224x224)
python prepare_calib_data.py

# INT8 Quantisierung nach ESP-DL
python quantize_onnx_model.py
```

Ergebnis: `quantized_model/espdet_pico_224_224_bumblebee.espdl`

## Hinweise

- Für neue Trainingsläufe Pfade in den Skripten ggf. auf den gewünschten Run anpassen.
- Wenn du das Datenset erweitert hast, zuerst Kalibrierungsdaten neu erzeugen.
