import json
import sys
import os
import re
from gc import enable


def remove_comments(json_str):
    pattern = r'(//[^\n]*)|(/\*[\s\S]*?\*/)'
    return re.sub(pattern, '', json_str)

def generate_header(json_path, header_path):
    try:
        with open(json_path, 'r') as f:
            raw_content = f.read()

        # Clean comments before parsing
        clean_content = remove_comments(raw_content)
        data = json.loads(clean_content)
    except Exception as e:
        print(f"Error reading JSON: {e}")
        sys.exit(1)

    # --- Extract Data ---
    matrix = data.get("matrix", {})
    row_pins = matrix.get("row_pins", [])
    col_pins = matrix.get("col_pins", [])
    direction = matrix.get("diode_direction", "COL2ROW")

    encoders = data.get("encoders", {})
    enable_encoder = encoders.get("enabled", False)
    encoder_pins = encoders.get("pins", [])

    # meta/usb sections
    meta = data.get("meta", {})
    usb  = data.get("usb", {})

    # Fallback logic: Try 'usb' section first, then 'meta', then defaults
    manufacturer = usb.get("manufacturer", meta.get("manufacturer", "Woyta Factory"))
    product = usb.get("product", meta.get("name", "Woyta Pad"))

    vid = usb.get("vid", "0xCAFE")
    pid = usb.get("pid", "0x4242")
    bcd_device = usb.get("device_ver", "0x0100")

    oled = data.get("oled", {})
    enable_oled = oled.get("enabled", False)
    oled_sda = oled.get("oled_sda")
    oled_scl = oled.get("oled_scl")


    content = [
        "// AUTO-GENERATED FILE. DO NOT EDIT.",
        "#pragma once",
        "",
        "// USB Identity",
        f'#define USB_MANUFACTURER "{manufacturer}"',
        f'#define USB_PRODUCT      "{product}"',
        f'#define USB_VID          {vid}',
        f'#define USB_PID          {pid}',
        f'#define USB_BCD          {bcd_device}',
        "",
        "// Hardware Definitions",
        f"#define MATRIX_ROWS {len(row_pins)}",
        f"#define MATRIX_COLS {len(col_pins)}",
        "",
        f"#define MATRIX_ROW_PINS {{ {', '.join(map(str, row_pins))} }}",
        f"#define MATRIX_COL_PINS {{ {', '.join(map(str, col_pins))} }}",
        "",
        f"#define DIODE_DIRECTION_{direction}",
        "",
    ]

    if enable_encoder and encoder_pins:
        content.append("#define ENABLE_ENCODER true")
        content.append(f"#define ENCODER_COUNT {len(encoder_pins)}")

        pad_a_pins = [enc.get("pad_a", 0) for enc in encoder_pins]
        pad_b_pins = [enc.get("pad_b", 0) for enc in encoder_pins]
        click_pins = [enc.get("click_pin", 0) for enc in encoder_pins]
        resolutions = [enc.get("resolution", 4) for enc in encoder_pins]

        content.append(f"#define ENCODER_PAD_A_PINS {{ {', '.join(map(str, pad_a_pins))} }}")
        content.append(f"#define ENCODER_PAD_B_PINS {{ {', '.join(map(str, pad_b_pins))} }}")
        content.append(f"#define ENCODER_CLICK_PINS {{ {', '.join(map(str, click_pins))} }}")
        content.append(f"#define ENCODER_RESOLUTIONS {{ {', '.join(map(str, resolutions))} }}")
    else:
        content.append("#define ENABLE_ENCODER false")

    if enable_oled and oled_sda and oled_scl:
        oled_width = oled.get("width", 128)
        oled_height = oled.get("height", 32)

        content.append("")
        content.append("#define ENABLE_OLED true")
        content.append(f"#define OLED_SDA_PIN {oled_sda}")
        content.append(f"#define OLED_SCL_PIN {oled_scl}")
        content.append(f"#define OLED_WIDTH {oled_width}")
        content.append(f"#define OLED_HEIGHT {oled_height}")
    else:
        content.append("")
        content.append("#define ENABLE_OLED false")

    physical_layout = data.get("physical_layout", [])
    if physical_layout:
        content.append("")
        content.append("// --- Physical Layout Geometry ---")
        content.append(f"#define LAYOUT_ITEM_COUNT {len(physical_layout)}")

        layout_bytes = []
        for item in physical_layout:
            x = item.get("x", 0)
            y = item.get("y", 0)
            item_type = item.get("type", "key")

            if item_type == "key":
                matrix_coords = item.get("matrix", [0, 0])
                # Format: Type (0=Key), X, Y, Matrix Row, Matrix Col
                layout_bytes.extend([0, x, y, matrix_coords[0], matrix_coords[1]])
            elif item_type == "encoder":
                index = item.get("index", 0)
                # Format: Type (1=Encoder), X, Y, Encoder Index, Padding (0)
                layout_bytes.extend([1, x, y, index, 0])

        # Format the byte array for C
        byte_strings = [str(b) for b in layout_bytes]
        content.append(f"#define PHYSICAL_LAYOUT_DATA {{ {', '.join(byte_strings)} }}")
    else:
        content.append("")
        content.append("#define LAYOUT_ITEM_COUNT 0")

    content.append("")

    with open(header_path, 'w') as f:
        f.write('\n'.join(content))

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python generate_config.py <input.json> <output.h>")
        sys.exit(1)

    generate_header(sys.argv[1], sys.argv[2])