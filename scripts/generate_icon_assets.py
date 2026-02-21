from PIL import Image, ImageDraw
import os

def generate_icons(source_path, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Open the base image
    base_img = Image.open(source_path).convert("RGBA")
    width, height = base_img.size
    
    # Create a rounded rectangle mask to remove white corners
    # The image is already a rounded square, but with white corners. 
    # We want to mask just the rounded square part.
    mask = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(mask)
    
    # Radius for rounded corners (approx 15-20% of width is standard for app icons)
    radius = int(width * 0.176) 
    draw.rounded_rectangle((0, 0, width, height), radius=radius, fill=255)
    
    # Apply mask to alpha channel
    base_img.putalpha(mask)
    
    # Standard resolutions
    sizes = [16, 32, 48, 256, 512, 1024]
    
    for size in sizes:
        resized = base_img.resize((size, size), Image.Resampling.LANCZOS)
        path = os.path.join(output_dir, f"icon_{size}.png")
        resized.save(path)
        print(f"Generated transparent: {path}")

    # For the root icon.png used by JUCE
    base_img.save(os.path.join(output_dir, "icon.png"))

    # Generate .ico for Windows (contains multiple sizes)
    ico_img = Image.open(source_path).convert("RGBA")
    mask_ico = Image.new("L", ico_img.size, 0)
    draw_ico = ImageDraw.Draw(mask_ico)
    draw_ico.rounded_rectangle((0, 0, ico_img.size[0], ico_img.size[1]), radius=int(ico_img.size[0]*0.176), fill=255)
    ico_img.putalpha(mask_ico)
    
    ico_path = os.path.join(output_dir, "icon.ico")
    ico_img.save(ico_path, format="ICO", sizes=[(16, 16), (32, 32), (48, 48), (256, 256)])
    print(f"Generated transparent ICO: {ico_path}")

if __name__ == "__main__":
    # We use the high-res icon as the source for transparency fix
    src = "/home/walisson/SolfeggioProcessor/Resources/icon_1024.png"
    out = "/home/walisson/SolfeggioProcessor/Resources"
    generate_icons(src, out)
