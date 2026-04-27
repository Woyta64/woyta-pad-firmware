#include <stdio.h>
#include <string.h>
#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "ssd1306.h"
#include "textRenderer/TextRenderer.h"
#include "generated_config.h"
#include "oled_assets.h"

#if ENABLE_OLED

using namespace pico_ssd1306;

#if (OLED_SDA_PIN == 0) || (OLED_SDA_PIN == 4) || (OLED_SDA_PIN == 8) || \
    (OLED_SDA_PIN == 12) || (OLED_SDA_PIN == 16) || (OLED_SDA_PIN == 20)
    #define OLED_I2C_INST i2c0
#else
    #define OLED_I2C_INST i2c1
#endif

static SSD1306 *display = nullptr;

// State machine
static oled_state_t current_state = OLED_STATE_BOOT;
static uint32_t state_enter_ms = 0;
static uint32_t last_activity_ms = 0;

// Animation
static uint8_t anim_frame = 0;
static uint32_t last_frame_ms = 0;

// Active state change tracking
static uint8_t last_rendered_layer = 0xFF;
static bool display_off = false;

// Render one frame of a bitmap animation to screen.
// Bitmaps are in SSD1306 vertical page format: each byte = 8 vertical pixels.
// Frame N starts at offset N * (width * pages) in the bitmap array.
static void render_bitmap_frame(const oled_anim_t *anim, uint8_t frame_idx) {
    uint16_t frame_size = OLED_WIDTH * (OLED_HEIGHT / 8);
    const uint8_t *frame_data = anim->bitmap + (frame_idx * frame_size);

    display->clear();
    for (int page = 0; page < OLED_HEIGHT / 8; page++) {
        for (int x = 0; x < OLED_WIDTH; x++) {
            uint8_t byte = frame_data[page * OLED_WIDTH + x];
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    display->setPixel(x, page * 8 + bit);
                }
            }
        }
    }
    display->sendBuffer();
}

static void change_state(oled_state_t new_state) {
    current_state = new_state;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    state_enter_ms = now;
    anim_frame = 0;
    last_frame_ms = now;
    last_rendered_layer = 0xFF; // force redraw on next active frame

    if (new_state == OLED_STATE_SLEEP) {
        display->clear();
        display->sendBuffer();
        display->turnOff();
        display_off = true;
    } else if (display_off) {
        display->turnOn();
        display_off = false;
    }
}

void oled_init_wrapper(void) {
    i2c_init(OLED_I2C_INST, 400 * 1000);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

#if OLED_HEIGHT == 64
    display = new SSD1306(OLED_I2C_INST, 0x3C, Size::W128xH64);
#else
    display = new SSD1306(OLED_I2C_INST, 0x3C, Size::W128xH32);
#endif

#if OLED_FLIP
    display->setOrientation(false);
#endif

    uint32_t now = to_ms_since_boot(get_absolute_time());
    state_enter_ms = now;
    last_activity_ms = now;
    last_frame_ms = now;

    // Show boot screen immediately
    const oled_anim_t *boot_anim = anim_lookup[OLED_STATE_BOOT];
    if (boot_anim) {
        render_bitmap_frame(boot_anim, 0);
    } else {
        // No boot image — show keyboard name
        display->clear();
        drawText(display, font_12x16, USB_PRODUCT, 0, 8);
        display->sendBuffer();
    }
}

void oled_task(uint8_t layer, bool key_pressed) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (key_pressed) {
        last_activity_ms = now;
    }

    uint32_t time_in_state = now - state_enter_ms;
    uint32_t idle_time = now - last_activity_ms;

    // --- State transitions ---
    switch (current_state) {
        case OLED_STATE_BOOT: {
            const oled_anim_t *anim = anim_lookup[OLED_STATE_BOOT];
            uint16_t duration = anim ? anim->frame_ms : OLED_BOOT_DURATION_MS;
            if (time_in_state >= duration) {
                change_state(OLED_STATE_ACTIVE);
            } else {
                return; // still showing boot screen
            }
            break;
        }
        case OLED_STATE_ACTIVE:
            if (idle_time >= OLED_SLEEP_TIMEOUT_MS) {
                change_state(OLED_STATE_SLEEP);
                return;
            }
            if (idle_time >= OLED_IDLE_TIMEOUT_MS) {
                change_state(OLED_STATE_IDLE);
            }
            break;

        case OLED_STATE_IDLE:
            if (key_pressed) {
                change_state(OLED_STATE_ACTIVE);
            } else if (idle_time >= OLED_SLEEP_TIMEOUT_MS) {
                change_state(OLED_STATE_SLEEP);
                return;
            }
            break;

        case OLED_STATE_SLEEP:
            if (key_pressed) {
                change_state(OLED_STATE_ACTIVE);
            } else {
                return; // display off, nothing to do
            }
            break;

        default:
            break;
    }

    // --- Rendering ---
    switch (current_state) {
        case OLED_STATE_ACTIVE: {
            // Only redraw when layer changes
            if (layer == last_rendered_layer) break;
            last_rendered_layer = layer;

            if (layer < MAX_LAYER_IMAGES && layer_images[layer]) {
                render_bitmap_frame(layer_images[layer], 0);
            }
            break;
        }

        case OLED_STATE_IDLE: {
            const oled_anim_t *anim = anim_lookup[OLED_STATE_IDLE];
            if (!anim) break;
            if (now - last_frame_ms >= anim->frame_ms) {
                render_bitmap_frame(anim, anim_frame);
                anim_frame = (anim_frame + 1) % anim->frames;
                last_frame_ms = now;
            }
            break;
        }

        default:
            break;
    }
}

#else
void oled_init_wrapper(void) {}
void oled_task(uint8_t layer, bool key_pressed) {}
#endif
