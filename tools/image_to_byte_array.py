from PIL import Image
import glob
import os
import sys

def convert_image_to_ssd1306_bytes(image_path):
    img = Image.open(image_path).convert('1')
    width, height = img.size

    data = []

    for page in range(0, height, 8):
        for x in range(width):
            byte = 0
            for bit in range(8):
                y = page + bit
                if y < height:
                    pixel = img.getpixel((x, y))
                    if pixel > 128:
                        byte |= (1 << bit)

            data.append(byte)

    return data, width, height

def generate_header(assets_dir, output_file):
    print(f"Scanning {assets_dir} for .png files...")

    valid_extensions = ["*.png", "*.jpg", "*.jpeg", "*.bmp"]

    image_files = []
    for ext in valid_extensions:
        image_files.extend(glob.glob(os.path.join(assets_dir, ext)))

    if not image_files:
        print(f"No image files found in {assets_dir}")

    image_files.sort()

    content = [
        "// AUTO-GENERATED FILE. DO NOT EDIT.",
        "#pragma once",
        "#include <stdint.h>",
        ""
    ]

    for img_path in image_files:
        filename = os.path.basename(img_path)
        # Extract name without extension (e.g. "my-logo.bmp" -> "my-logo")
        name_no_ext = os.path.splitext(filename)[0]

        # Sanitize: Replace spaces/dashes with underscores to make valid C syntax
        safe_name = name_no_ext.replace(" ", "_").replace("-", "_").lower()
        var_name = f"bitmap_{safe_name}"

        data, w, h = convert_image_to_ssd1306_bytes(img_path)

        # Write C Array
        content.append(f"// Source: {filename} ({w}x{h})")
        # Note: We don't use 'static' here so assets.h can see it
        content.append(f"const uint8_t {var_name}[] = {{")

        # Formatting hex nicely (16 bytes per line)
        hex_lines = []
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_chunk = ", ".join(f"0x{b:02X}" for b in chunk)
            hex_lines.append(f"    {hex_chunk}")

        content.append(",\n".join(hex_lines))
        content.append("};\n")

        print(f"Converted {filename} -> {var_name} ({len(data)} bytes)")

    # Write to file
    with open(output_file, 'w') as f:
        f.write('\n'.join(content))

if __name__ == "__main__":
    # Check if we have the correct number of arguments (Script + Input Dir + Output File)
    if len(sys.argv) != 3:
        print("Usage: python image_to_byte_array.py <assets_dir> <output_header>")
        sys.exit(1)

    # Run the generator with arguments passed from CMake
    generate_header(sys.argv[1], sys.argv[2])