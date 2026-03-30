# Bumblebee Detect Component

Dieses Component kapselt das Bumblebee-Erkennungsmodell für ESP-DL.

## Enthaltene Dateien

- `bumblebee_detect.hpp/.cpp` – Wrapper-Klasse `BumblebeeDetect`
- `espdet_pico_224_224_bumblebee.espdl` – Modellartefakt
- `Kconfig` – Auswahl von Modellspeicher und Flash-Optionen

## Verwendung im Code

```cpp
#include "bumblebee_detect.hpp"

BumblebeeDetect *detect = new BumblebeeDetect();
dl::image::img_t img = {.data = DATA, .width = WIDTH, .height = HEIGHT, .pix_type = PIX_TYPE};
auto &results = detect->run(img);
```

## Konfiguration (menuconfig)

Siehe `Kconfig` im gleichen Verzeichnis.

Wichtige Optionen:

- `CONFIG_FLASH_ESPDET_PICO_224_224_BUMBLEBEE`
- `CONFIG_BUMBLEBEE_DETECT_MODEL_IN_FLASH_RODATA`
- `CONFIG_BUMBLEBEE_DETECT_MODEL_IN_FLASH_PARTITION`
- `CONFIG_BUMBLEBEE_DETECT_MODEL_IN_SDCARD`
- `CONFIG_BUMBLEBEE_DETECT_MODEL_SDCARD_DIR`

## Modellspeicher

Das Modell kann aus drei Orten geladen werden:

1. Flash rodata
2. Flash-Partition (`bumblebee_det`)
3. SD-Karte

Wenn `FLASH_PARTITION` aktiv ist, muss die Partitionstabelle eine ausreichend große Partition für das Modell enthalten.