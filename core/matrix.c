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
#ifdef DIODE_DIRECTION_COL2ROW
    // COL2ROW: drive columns, read rows
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
#else
    // ROW2COL: drive rows, read columns
    for (int i = 0; i < MATRIX_COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_IN);
        gpio_pull_down(col_pins[i]);
    }
    for (int i = 0; i < MATRIX_ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_IN);
        gpio_disable_pulls(row_pins[i]);
    }
#endif
}

void matrix_scan(void) {
#ifdef DIODE_DIRECTION_COL2ROW
    // COL2ROW: drive each column, read rows
    for (int c = 0; c < MATRIX_COLS; c++) {
        gpio_set_dir(col_pins[c], GPIO_OUT);
        gpio_put(col_pins[c], 1);
        sleep_us(30);

        for (int r = 0; r < MATRIX_ROWS; r++) {
            matrix_state[r][c] = gpio_get(row_pins[r]);
        }

        gpio_put(col_pins[c], 0);
        gpio_set_dir(col_pins[c], GPIO_IN);
        sleep_us(30);
    }
#else
    // ROW2COL: drive each row, read columns
    for (int r = 0; r < MATRIX_ROWS; r++) {
        gpio_set_dir(row_pins[r], GPIO_OUT);
        gpio_put(row_pins[r], 1);
        sleep_us(30);

        for (int c = 0; c < MATRIX_COLS; c++) {
            matrix_state[r][c] = gpio_get(col_pins[c]);
        }

        gpio_put(row_pins[r], 0);
        gpio_set_dir(row_pins[r], GPIO_IN);
        sleep_us(30);
    }
#endif
}

bool matrix_is_on(uint8_t row, uint8_t col) {
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) return false;
    return matrix_state[row][col];
}
