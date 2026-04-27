# Woyta-Pad Firmware

RP2040-based macropad firmware with USB HID support. Configurable wirelessly via a companion [WebHID configurator](https://github.com/woyta64/woyta-pad-frontend).

## Features

- Composite USB HID device (keyboard + WebHID config channel)
- Key matrix scanning with anti-spam
- Rotary encoder support
- OLED display support (SSD1306, I2C)
- Up to 128 macros
- Multi-layer keymaps (remappable via WebHID at runtime)
- Per-keyboard configuration via JSON

## Prerequisites

- **ARM GCC toolchain** (`arm-none-eabi-gcc`)
- **CMake** >= 3.13
- **Python 3** (for code generation scripts)
- **Pillow** Python package (only if using OLED assets)

### Install on Arch Linux

```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib cmake python
```

### Install on Ubuntu/Debian

```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi cmake python3
```

## Getting Started

### Clone

```bash
git clone --recurse-submodules https://github.com/woyta/woyta-pad-fw.git
cd woyta-pad-fw
```

If you already cloned without `--recurse-submodules`:

```bash
git submodule update --init --recursive
```

This pulls in `lib/pico-sdk` and `lib/pico-ssd1306`.

### Set up Python venv (optional, for OLED asset generation)

```bash
python3 -m venv venv
source venv/bin/activate
pip install Pillow
```

The build system auto-detects `venv/bin/python3` if present, otherwise falls back to system `python3`.

### Build

```bash
export PICO_SDK_PATH=$(pwd)/lib/pico-sdk
mkdir -p build && cd build
cmake -Dkb=woyta-pad ..
make -j$(nproc)
```

Replace `woyta-pad` with your keyboard folder name. If `-Dkb` is omitted, it defaults to `woyta-pad`.

The output `woyta_pad.uf2` will be in the `build/` directory.

### Flash

1. Hold the **BOOTSEL** button on your RP2040 board
2. Plug in USB (or reset while holding BOOTSEL)
3. Copy the `.uf2` file to the mounted `RPI-RP2` drive:

```bash
cp woyta_pad.uf2 /run/media/$USER/RPI-RP2/
```

## Creating a New Keyboard

Each keyboard lives in its own folder under `keyboards/`.

### Required files

```
keyboards/your-board/
  keyboard.json       # Hardware definition
  keymap.c            # Default keymap
```

### Optional files (only for OLED-enabled boards)

```
keyboards/your-board/
  assets.h            # OLED timing overrides
  assets/             # OLED images (PNG/JPG/BMP), auto-converted at build time
```

---

### Step 1: Create the folder

```bash
mkdir -p keyboards/your-board
```

### Step 2: Create `keyboard.json`

This defines your hardware. Only `matrix` and `physical_layout` are required — everything else has sensible defaults.

Here's a full-featured example (3x3 matrix + 1 encoder + OLED). See `keyboards/example-3x3/` for this exact setup:

```json
{
  "meta": {
    "name": "Your Board",
    "version": "1.0.0",
    "maintainer": "You"
  },
  "matrix": {
    "rows": 3,
    "cols": 3,
    "row_pins": [5, 6, 7],
    "col_pins": [8, 9, 10],
    "diode_direction": "COL2ROW"
  },
  "physical_layout": [
    { "x": 0, "y": 0, "type": "encoder", "index": 0 },
    { "x": 0, "y": 1, "type": "key", "matrix": [0, 0] },
    { "x": 1, "y": 1, "type": "key", "matrix": [0, 1] },
    { "x": 2, "y": 1, "type": "key", "matrix": [0, 2] },
    { "x": 0, "y": 2, "type": "key", "matrix": [1, 0] },
    { "x": 1, "y": 2, "type": "key", "matrix": [1, 1] },
    { "x": 2, "y": 2, "type": "key", "matrix": [1, 2] },
    { "x": 0, "y": 3, "type": "key", "matrix": [2, 0] },
    { "x": 1, "y": 3, "type": "key", "matrix": [2, 1] },
    { "x": 2, "y": 3, "type": "key", "matrix": [2, 2] }
  ],
  "encoders": {
    "enabled": true,
    "pins": [
      { "pad_a": 14, "pad_b": 15, "click_pin": 16, "resolution": 4 }
    ]
  },
  "oled": {
    "enabled": true,
    "width": 128,
    "height": 32,
    "oled_sda": 0,
    "oled_scl": 1,
    "flip": false
  }
}
```

Don't need encoders or OLED? Just omit those sections entirely — they default to disabled. Same for `meta` and `usb`.

#### `keyboard.json` reference

**Required sections:**

| Section | Field | Description |
|---------|-------|-------------|
| `matrix` | `rows`, `cols` | Matrix dimensions |
| `matrix` | `row_pins`, `col_pins` | RP2040 GPIO pin numbers |
| `matrix` | `diode_direction` | `"COL2ROW"` or `"ROW2COL"` |
| `physical_layout[]` | | Visual position of each key/encoder for the WebHID configurator |

Each `physical_layout` entry:

| Field | Required | Description |
|-------|----------|-------------|
| `x`, `y` | yes | Grid position |
| `type` | yes | `"key"` or `"encoder"` |
| `matrix` | keys only | `[row, col]` index into the matrix |
| `index` | encoders only | Encoder index (0-based) |

**Optional sections:**

| Section | Field | Default | Description |
|---------|-------|---------|-------------|
| `meta` | `name` | `"Woyta Pad"` | Display name |
| `meta` | `version` | — | Version string |
| `meta` | `maintainer` | — | Author |
| `usb` | `vid` | `0xCAFE` | USB Vendor ID |
| `usb` | `pid` | `0x4242` | USB Product ID |
| `usb` | `device_ver` | `0x0100` | USB device version |
| `usb` | `manufacturer` | `"Woyta Factory"` | USB manufacturer string |
| `usb` | `product` | from `meta.name` | USB product string |
| `encoders` | `enabled` | `false` | Enable rotary encoders |
| `encoders` | `pins[]` | `[]` | Array of encoder pin definitions |
| `oled` | `enabled` | `false` | Enable OLED display |
| `oled` | `width` / `height` | `128` / `32` | Display resolution |
| `oled` | `oled_sda` / `oled_scl` | — | I2C GPIO pins (required if OLED enabled) |
| `oled` | `flip` | `false` | Flip display 180 degrees |

### Step 3: Create `keymap.c`

Defines default layers. Must include `keycodes.h` and `generated_config.h`.

```c
#include <stdint.h>
#include "keycodes.h"
#include "generated_config.h"

#define LAYERS 2

// MATRIX KEYMAP
// Structure: [Layer][Row][Column]
uint16_t keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS] = {

    // Layer 0: Numpad
    [0] = {
        { KC_7, KC_8, KC_9 },
        { KC_4, KC_5, KC_6 },
        { KC_1, KC_2, KC_3 }
    },

    // Layer 1: Media
    [1] = {
        { KC_MEDIA_PREVIOUSSONG, KC_MEDIA_PLAYPAUSE, KC_MEDIA_NEXTSONG },
        { KC_MEDIA_VOLUMEDOWN,   KC_MEDIA_MUTE,      KC_MEDIA_VOLUMEUP },
        { KC_NONE,               KC_NONE,             KC_NONE }
    }
};

// ENCODER MAP
// Structure: [Layer][Encoder Index][CW, CCW, Click]
uint16_t encoder_map[LAYERS][ENCODER_COUNT][3] = {

    [0] = {
        { KC_MEDIA_VOLUMEUP, KC_MEDIA_VOLUMEDOWN, KC_MEDIA_MUTE }
    },

    [1] = {
        { KC_RIGHT, KC_LEFT, KC_LAY_NEXT }
    }
};
```

Rules:
- Array dimensions must match: `keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS]`
- Each row in a layer must have exactly `MATRIX_COLS` entries
- `MATRIX_ROWS`, `MATRIX_COLS`, and `ENCODER_COUNT` come from `generated_config.h` (auto-generated from your `keyboard.json`)
- If you have no encoders, just omit the `encoder_map` — the firmware provides a default automatically

#### Useful keycodes

| Keycode | Description |
|---------|-------------|
| `KC_NONE` / `KC_TRNS` | No action / transparent (falls through to layer 0) |
| `KC_LAY_NEXT` | Cycle to next layer |
| `KC_A` .. `KC_Z` | Letters |
| `KC_1` .. `KC_0` | Numbers |
| `KC_ENTER`, `KC_SPACE`, `KC_BACKSPACE` | Common keys |
| `KC_F1` .. `KC_F24` | Function keys |
| `KC_MEDIA_VOLUMEUP/DOWN`, `KC_MEDIA_MUTE` | Media controls |
| `KC_MEDIA_PLAYPAUSE`, `KC_MEDIA_NEXTSONG` | Playback |
| `KC_MACRO_0` .. `KC_MACRO_127` | Trigger macro slots |

Full list in `core/keycodes.h`.

### Step 4: Add encoders (optional)

In `keyboard.json`, add the `encoders` section:

```json
"encoders": {
  "enabled": true,
  "pins": [
    { "pad_a": 14, "pad_b": 15, "click_pin": 29, "resolution": 4 }
  ]
}
```

Add encoder entries to `physical_layout`:

```json
{ "x": 0, "y": 0, "type": "encoder", "index": 0 }
```

In `keymap.c`, define the encoder map:

```c
// Structure: [Layer][Encoder Index][CW, CCW, Click]
uint16_t encoder_map[LAYERS][ENCODER_COUNT][3] = {
    [0] = {
        { KC_MEDIA_VOLUMEUP, KC_MEDIA_VOLUMEDOWN, KC_MEDIA_MUTE }
    },
    [1] = {
        { KC_RIGHT, KC_LEFT, KC_ENTER }
    }
};
```

### Step 5: Add OLED (optional)

In `keyboard.json`, add the `oled` section:

```json
"oled": {
  "enabled": true,
  "width": 128,
  "height": 32,
  "oled_sda": 0,
  "oled_scl": 1,
  "flip": true
}
```

#### OLED assets

Place images in `keyboards/your-board/assets/`. They get auto-converted to C byte arrays at build time. Supported formats: PNG, JPG, BMP. All images must be 128x32 1-bit (black and white).

| Filename | Purpose | Required |
|----------|---------|----------|
| `boot.png` | Shown on startup | No (falls back to keyboard name as text) |
| `layer1.png` .. `layer4.png` | Shown when switching layers | No (falls back to built-in defaults) |
| `idle.png` | Idle animation (frames stacked vertically, 128xN) | No (falls back to built-in animation) |

Example assets directory:

```
keyboards/your-board/assets/
  boot.png       # 128x32 — boot splash
  layer1.png     # 128x32 — layer 1 indicator
  layer2.png     # 128x32 — layer 2 indicator
```

#### OLED timing overrides

Optionally create `assets.h` to tweak timing:

```c
#pragma once

// Uncomment to override defaults:
// #define OLED_IDLE_TIMEOUT_MS  10000   // ms idle before idle animation
// #define OLED_SLEEP_TIMEOUT_MS 60000   // ms idle before display off
// #define OLED_BOOT_DURATION_MS 3000    // ms to show boot screen
// #define OLED_IDLE_FRAME_MS    150     // ms per idle animation frame
```

### Step 6: Build

```bash
cd build
cmake -Dkb=your-board ..
make -j$(nproc)
```

## Example Keyboard

A complete full-featured example lives in `keyboards/example-3x3/`:

```
keyboards/example-3x3/
  keyboard.json       # 3x3 matrix + 1 encoder + OLED
  keymap.c            # 2 layers (numpad + media)
  assets.h            # OLED timing overrides (all defaults)
  assets/
    boot.png          # Boot splash screen
    layer1.png        # Layer 1 display
    layer2.png        # Layer 2 display
```

Use it as a starting point — copy the folder, rename, and edit to match your hardware.