# Bumblebee Detection v2 (Flight Tracking)

Version 2 erweitert die reine Erkennung um einfache Flugzählung:

- Bounding-Box-Erkennung auf jedem Frame
- Mittelpunkt-Tracking zwischen aufeinanderfolgenden Frames
- Zählung von **Einflug** / **Ausflug** über eine definierte Linie
- Speicherung annotierter Bilder auf SD-Karte

![](./img/bumblebee_tracking_224_224.gif)

## Zähllogik (vereinfacht)

- Eine horizontale Linie (`y_line`) trennt Ein-/Ausflug
- Mittelpunkt wechselt über die Linie → Zähler wird erhöht
- Ergebnisse werden im Serial Monitor ausgegeben

## Build & Flash

```powershell
cd hardware/firmware/bumblebee_detection/v2
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

## Hinweise

- Modellkomponente: `main/bumblebee_detect`
- SD-Karte erforderlich für Bildspeicherung
- Kein Netzwerk-Upload in dieser Version