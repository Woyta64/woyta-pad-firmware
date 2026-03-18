#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "matrix.h"
#include "encoder.h"
#include "generated_config.h"
#include "keycodes.h"
#include "oled.h"

#define MAX_LAYERS 4

// Import maps from keymap.c
extern uint8_t keymap[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];
extern uint8_t encoder_map[MAX_LAYERS][ENCODER_COUNT][3];

// Buffer for outgoing WebHID data
uint8_t report_buffer[32];

// --- BURST MODE STATE MACHINE ---
typedef enum {
    BURST_IDLE,
    BURST_SEND_START,
    BURST_SEND_CHUNKS,
    BURST_SEND_END
} burst_state_t;

burst_state_t current_burst_state = BURST_IDLE;
uint8_t current_layout_offset = 0;

// WebHID flags
bool has_metadata_response = false;

// --- TINYUSB CALLBACKS ---

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    if (instance == 0) {
        memset(buffer, 0, 8);
        return 8;
    }
    if (instance == 1) {
        memset(buffer, 0, 32);
        return 32;
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    if (instance != 1 || bufsize < 1) return;

    uint8_t command = buffer[0];

    if (command == 0x00) {
        // Dummy packet for Linux toggle sync. Do nothing.
    }
    else if (command == 0x01) {
        has_metadata_response = true;
    }
    else if (command == 0x02) {
        current_burst_state = BURST_SEND_START;
        current_layout_offset = 0;
    }
}

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

#if ENABLE_OLED
    oled_init_wrapper();
#endif

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

#if LAYOUT_ITEM_COUNT > 0
    uint8_t layout_data[] = PHYSICAL_LAYOUT_DATA;
    uint16_t total_layout_bytes = sizeof(layout_data);
#else
    uint16_t total_layout_bytes = 0;
#endif

    while (true) {
        tud_task();
        matrix_scan();

        // --- 1. WEBHID CONFIGURATOR LOGIC (Interface 1) ---
        if (tud_hid_n_ready(1)) {

            if (has_metadata_response) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x01;
                report_buffer[1] = MATRIX_ROWS;
                report_buffer[2] = MATRIX_COLS;
                report_buffer[3] = MAX_LAYERS;
                tud_hid_n_report(1, 0, report_buffer, 32);
                has_metadata_response = false;
            }
            else if (current_burst_state == BURST_SEND_START) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x12;
                report_buffer[1] = total_layout_bytes / 5;
                tud_hid_n_report(1, 0, report_buffer, 32);
                current_burst_state = BURST_SEND_CHUNKS;
            }
            else if (current_burst_state == BURST_SEND_CHUNKS) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x02;
                report_buffer[1] = current_layout_offset;

#if LAYOUT_ITEM_COUNT > 0
                for(int i = 0; i < 30 && (current_layout_offset + i) < total_layout_bytes; i++) {
                    report_buffer[i+2] = layout_data[current_layout_offset + i];
                }
#endif

                tud_hid_n_report(1, 0, report_buffer, 32);
                current_layout_offset += 30;

                if (current_layout_offset >= total_layout_bytes) {
                    current_burst_state = BURST_SEND_END;
                }
            }
            else if (current_burst_state == BURST_SEND_END) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x22;
                tud_hid_n_report(1, 0, report_buffer, 32);
                current_burst_state = BURST_IDLE;
            }
        }

        // --- 2. STANDARD KEYBOARD LOGIC (Interface 0) ---
        bool any_key_pressed = false;

        // Default interface is 0 for the standard keyboard
        if (tud_hid_ready()) {
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

#if ENABLE_OLED
        oled_task(current_layer, any_key_pressed);
#endif

    }
    return 0;
}
