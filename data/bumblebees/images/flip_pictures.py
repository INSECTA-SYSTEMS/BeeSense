import os
from pathlib import Path
from PIL import Image

def flip_images_vertically(input_folder, output_folder=None):
    """
    Vertically flips all images from the input folder.
    
    Args:
        input_folder: Path to the folder containing raw images
        output_folder: Path to save flipped images (defaults to parent of input_folder)
    """
    input_path = Path(input_folder)
    
    # If no output folder specified, use parent directory of input folder
    if output_folder is None:
        output_folder = input_path.parent
    else:
        output_folder = Path(output_folder)
    
    # Create output folder if it doesn't exist
    output_folder.mkdir(parents=True, exist_ok=True)
    
    # Supported image extensions
    image_extensions = {'.jpg', '.jpeg', '.png', '.bmp', '.gif', '.tiff', '.webp'}
    
    # Process all images in the input folder
    image_files = [f for f in input_path.iterdir() 
                   if f.is_file() and f.suffix.lower() in image_extensions]
    
    if not image_files:
        print(f"Keine Bilder in {input_folder} gefunden.")
        return
    
    print(f"{len(image_files)} Bilder gefunden. Beginne horizontales Spiegeln...")
    
    for image_file in image_files:
        try:
            # Open image
            img = Image.open(image_file)
            
            # Flip horizontally (left to right)
            flipped_img = img.transpose(Image.FLIP_LEFT_RIGHT)
            
            # Save to output folder with _flip suffix
            output_filename = f"{image_file.stem}_flip{image_file.suffix}"
            output_path = output_folder / output_filename
            flipped_img.save(output_path)
            
            print(f"✓ {image_file.name} → {output_filename}")
            
        except Exception as e:
            print(f"✗ Fehler beim Verarbeiten von {image_file.name}: {e}")
    
    print(f"\nFertig! Gespiegelte Bilder gespeichert in: {output_folder}")


if __name__ == "__main__":
    # Path to raw folder
    script_dir = Path(__file__).parent
    pic_folder = script_dir / "raw"
    
    # Flip all images from raw folder and save to raw folder
    flip_images_vertically(pic_folder, pic_folder)
