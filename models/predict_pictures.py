from ultralytics import YOLO

# Modell laden
model = YOLO("runs/detect/train_224_224/weights/best.pt")

# Inferenz auf allen Testbildern im Ordner
results = model.predict(
    source="../data/bumblebees/images/test/",  # Pfad zum Test-Ordner
    conf=0.3,   # Confidence Threshold
    save=True,  # speichert Bilder mit Bounding Boxes
    show=False  # kein Fenster öffnen
)

# Ergebnisse ausgeben
print(f"\nInsgesamt {len(results)} Bilder getestet:\n")
for i, r in enumerate(results):
    print(f"Bild {i+1}: {len(r.boxes)} Hummeln gefunden")
    if len(r.boxes) > 0:
        print(f"  Konfidenz: {r.boxes.conf.tolist()}")
        
print(f"\nGespeichert in: runs/detect/predict/")
