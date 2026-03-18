#include <stdio.h>
#include <string.h>
#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "matrix.h"

// Library Includes
#include "ssd1306.h"

// Project Config
#include "generated_config.h"
#include "oled_types.h"
#include "assets.h"

#if OLED_ENABLED

// --- 1. GLOBAL OBJECTS (Must be outside functions) ---
static ssd1306_t disp;

// Select I2C Instance
#if (OLED_SDA_PIN == 0) || (OLED_SDA_PIN == 4) || (OLED_SDA_PIN == 8) || (OLED_SDA_PIN == 12) || (OLED_SDA_PIN == 16) || (OLED_SDA_PIN == 20)
    #define OLED_I2C_INST i2c0
#else
    #define OLED_I2C_INST i2c1
#endif

// --- 2. HELPER FUNCTIONS (Because the lib lacks draw_rect) ---

// Helper: Draw an empty box
void debug_draw_rect(int x, int y, int w, int h) {
    // Top & Bottom
    ssd1306_draw_line(&disp, x, y, x + w, y);
    ssd1306_draw_line(&disp, x, y + h, x + w, y + h);
    // Left & Right
    ssd1306_draw_line(&disp, x, y, x, y + h);
    ssd1306_draw_line(&disp, x + w, y, x + w, y + h);
}

// Helper: Draw a filled box
void debug_fill_rect(int x, int y, int w, int h) {
    for (int i = x; i <= x + w; i++) {
        ssd1306_draw_line(&disp, i, y, i, y + h);
    }
}

// --- 3. INIT WRAPPER ---
void oled_init_wrapper(void) {
    // Initialize I2C
    i2c_init(OLED_I2C_INST, 400 * 1000);

    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    // Initialize OLED (128x32)
    ssd1306_init(&disp, 128, 32, 0x3C, OLED_I2C_INST);

    ssd1306_clear(&disp);
    ssd1306_show(&disp);
}

// --- 4. DEBUG TASK (The Matrix Visualizer) ---
void oled_task(uint8_t layer, bool key_pressed) {
    // 1. Clear Screen
    ssd1306_clear(&disp);

    // 2. Draw Grid
    int cell_w = 12; // Width of one grid cell
    int cell_h = 6;  // Height (smaller to fit 32px screen)
    int start_x = 20;
    int start_y = 2;

    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            int x = start_x + (c * cell_w);
            int y = start_y + (r * cell_h);

            // Draw Outline
            debug_draw_rect(x, y, cell_w - 2, cell_h - 2);

            // If Pressed, Fill it
            if (matrix_is_on(r, c)) {
                debug_fill_rect(x, y, cell_w - 2, cell_h - 2);
            }
        }
    }

    // 3. Draw Debug Info
    char buf[16];
    sprintf(buf, "L:%d", layer);
    ssd1306_draw_string(&disp, 0, 24, 1, buf);

    // 4. Send to Display
    ssd1306_show(&disp);
}

#else
// Stubs for when OLED is disabled
void oled_init_wrapper(void) {}
void oled_task(uint8_t layer, bool key_pressed) {}
#endif