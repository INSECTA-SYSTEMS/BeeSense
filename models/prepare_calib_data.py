import os
import random
from PIL import Image

src_folder = "../data/bumblebees/images/train"  # oder val
dst_folder = "calib_data"
os.makedirs(dst_folder, exist_ok=True)

all_imgs = [f for f in os.listdir(src_folder) if f.endswith(('.jpg', '.png'))]
sample_imgs = random.sample(all_imgs, 32)  # z.B. 32 Bilder

for img in sample_imgs:
    src_path = os.path.join(src_folder, img)
    dst_path = os.path.join(dst_folder, img)
    with Image.open(src_path) as im:
        im = im.convert("RGB")
        im = im.resize((224, 224), Image.LANCZOS)
        im.save(dst_path)