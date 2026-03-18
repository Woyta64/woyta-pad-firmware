#include "matrix.h"
#include "pico/stdlib.h"
#include "generated_config.h"

// Hardware Pin Definitions
static const uint8_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const uint8_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

// State buffer
static bool matrix_state[MATRIX_ROWS][MATRIX_COLS];


// --- INITIALIZATION ---
void matrix_init(void) {
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
        gpio_pull_down(row_pins[i]);
    }
    for (int i = 0; i < MATRIX_COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_IN);
        gpio_disable_pulls(col_pins[i]);
    }
}

void matrix_scan(void) {
    for (int c = 0; c < MATRIX_COLS; c++) {
        // 1. Column HIGH (3.3V)
        gpio_set_dir(col_pins[c], GPIO_OUT);
        gpio_put(col_pins[c], 1);
        sleep_us(30);

        // 2. Read Rows
        for (int r = 0; r < MATRIX_ROWS; r++) {
            if (gpio_get(row_pins[r])) {
                matrix_state[r][c] = true;
            } else {
                matrix_state[r][c] = false;
            }
        }

        // 3. Turn Column OFF
        gpio_put(col_pins[c], 0);
        gpio_set_dir(col_pins[c], GPIO_IN);
        sleep_us(30);
    }
}

bool matrix_is_on(uint8_t row, uint8_t col) {
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return false;
    return matrix_state[row][col];
}