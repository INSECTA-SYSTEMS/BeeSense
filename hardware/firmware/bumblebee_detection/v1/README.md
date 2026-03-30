# Bumblebee Detection v1

Version 1 ist die Basis-Erkennung:

- kontinuierliche Kameraaufnahme
- Bumblebee-Erkennung mit `BumblebeeDetect`
- Speicherung annotierter Bilder auf SD-Karte
- **keine** Flugzählung und **kein** WiFi-Upload

## Build & Flash

```powershell
cd hardware/firmware/bumblebee_detection/v1
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

## Hinweise

- Für Modelloptionen siehe `main/bumblebee_detect/Kconfig`
- Wenn Modell aus Partition geladen wird, passende Partitionstabelle nutzen (`partitions2.csv`)
- Beispielausgaben und Bilder liegen in `img/`