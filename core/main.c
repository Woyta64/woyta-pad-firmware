#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "matrix.h"
#include "encoder.h"
#include "generated_config.h"
#include "keycodes.h"

#define MAX_LAYERS 4

// Import maps from keymap.c
extern uint8_t keymap[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];
extern uint8_t encoder_map[MAX_LAYERS][ENCODER_COUNT][3];

// --- CALLBACKS (Required for linking) ---
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

// --- HARDWARE AND MATRIX LOGIC ---

bool encoder_timer_callback(struct repeating_timer *t) {
#if ENCODER_COUNT > 0
    encoder_read();
#endif
    return true;
}

void process_layer_cycle(uint8_t *current_layer, bool *layer_key_was_pressed, bool *any_key_pressed) {
    bool layer_key_is_pressed = false;

    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (matrix_is_on(r, c)) {
                *any_key_pressed = true;
                uint8_t code = keymap[*current_layer][r][c];
                if (code == KC_TRNS) {
                    code = keymap[0][r][c];
                }
                if (code == KC_LAY_NEXT) {
                    layer_key_is_pressed = true;
                }
            }
        }
    }

    if (layer_key_is_pressed && !(*layer_key_was_pressed)) {
        (*current_layer)++;
        if (*current_layer >= MAX_LAYERS) {
            *current_layer = 0;
        }
    }
    *layer_key_was_pressed = layer_key_is_pressed;
}

void process_matrix_keys(uint8_t current_layer, uint8_t *keycode, uint8_t *modifier, int *count) {
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (matrix_is_on(r, c)) {
                uint8_t code = keymap[current_layer][r][c];
                if (code == KC_TRNS) {
                    code = keymap[0][r][c];
                }

                if (code == KC_LAY_NEXT) continue;

                if (code >= 0xE0 && code <= 0xE7) {
                    *modifier |= (1 << (code - 0xE0));
                }
                else if (code != 0 && *count < 6) {
                    keycode[*count] = code;
                    (*count)++;
                }
            }
        }
    }
}

void process_encoders(uint8_t current_layer, uint8_t *keycode, uint8_t *modifier, int *count, bool *any_key_pressed) {
#if ENCODER_COUNT > 0
    for (int i = 0; i < ENCODER_COUNT; i++) {
        int32_t delta = encoder_get_delta(i);
        if (delta != 0) {
            *any_key_pressed = true;
            if (*count < 6) {
                uint8_t enc_code = (delta > 0) ? encoder_map[current_layer][i][0] : encoder_map[current_layer][i][1];
                if (enc_code == KC_TRNS) {
                    enc_code = (delta > 0) ? encoder_map[0][i][0] : encoder_map[0][i][1];
                }
                if (enc_code != 0) {
                    keycode[*count] = enc_code;
                    (*count)++;
                }
            }
        }

        if (encoder_get_click(i)) {
            *any_key_pressed = true;
            uint8_t click_code = encoder_map[current_layer][i][2];
            if (click_code == KC_TRNS) {
                click_code = encoder_map[0][i][2];
            }
            if (click_code == KC_LAY_NEXT) continue;

            if (click_code >= 0xE0 && click_code <= 0xE7) {
                *modifier |= (1 << (click_code - 0xE0));
            }
            else if (click_code != 0 && *count < 6) {
                keycode[*count] = click_code;
                (*count)++;
            }
        }
    }
#endif
}

// --- MAIN LOOP ---

int main()
{
    stdio_init_all();
    matrix_init();
    tusb_init();

#if ENCODER_COUNT > 0
    encoder_init();
    struct repeating_timer timer;
    add_repeating_timer_ms(-1, encoder_timer_callback, NULL, &timer);
#endif

    uint8_t current_layer = 0;
    bool layer_key_was_pressed = false;

    // Tracks if the last USB report contained pressed keys.
    // Crucial for preventing USB traffic jams on the typing interface.
    bool previous_report_had_keys = false;

    while (true) {
        tud_task();
        matrix_scan();

        if (tud_hid_ready()) {
            bool any_key_pressed = false;
            uint8_t keycode[6] = {0};
            uint8_t modifier = 0;
            int count = 0;

            process_layer_cycle(&current_layer, &layer_key_was_pressed, &any_key_pressed);
            process_matrix_keys(current_layer, keycode, &modifier, &count);
            process_encoders(current_layer, keycode, &modifier, &count, &any_key_pressed);

            bool current_report_has_keys = (count > 0 || modifier > 0);

            // Only send data to the PC if a key is actively held down,
            // OR if we just let go and need to send the final empty "key release" packet.
            if (current_report_has_keys || previous_report_had_keys) {
                tud_hid_keyboard_report(0, modifier, keycode);
            }

            previous_report_had_keys = current_report_has_keys;
        }
    }
    return 0;
}
