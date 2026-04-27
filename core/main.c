#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "matrix.h"
#include "encoder.h"
#include "generated_config.h"
#include "keycodes.h"
#include "macro.h"
#include "keymap_store.h"
#include "oled.h"

// Import maps from keymap.c
extern uint16_t keymap[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];
#if ENCODER_COUNT > 0
extern uint16_t encoder_map[MAX_LAYERS][ENCODER_COUNT][3];
#else
static uint16_t encoder_map[1][1][3] = {0};
#endif

// Buffer for outgoing WebHID data
static uint8_t report_buffer[32];

// --- BURST MODE STATE MACHINE ---
typedef enum {
    BURST_IDLE,
    BURST_SEND_START,
    BURST_SEND_CHUNKS,
    BURST_SEND_END
} burst_state_t;

typedef enum {
    BURST_TYPE_LAYOUT,
    BURST_TYPE_KEYMAP,
    BURST_TYPE_MACRO
} burst_type_t;

static burst_state_t current_burst_state = BURST_IDLE;
static burst_type_t current_burst_type = BURST_TYPE_LAYOUT;
static uint16_t current_layout_offset = 0;
static uint8_t keymap_burst_layer = 0;
static uint8_t keymap_burst_chunk = 0;

// Macro burst state (0x05 Get Macro)
static uint8_t macro_burst_slot = 0;
static uint8_t macro_burst_chunk = 0;
#define MACRO_BURST_PAYLOAD 30
#define MACRO_BURST_CHUNK_COUNT ((MACRO_SLOT_SIZE + MACRO_BURST_PAYLOAD - 1) / MACRO_BURST_PAYLOAD)

// Macro receive buffer (0x06 Set Macro — multi-packet)
static uint8_t macro_rx_buf[MACRO_SLOT_SIZE];
static uint8_t macro_rx_slot = 0;
static uint8_t macro_rx_action_count = 0;

// WebHID flags
static bool has_metadata_response = false;

// Debounce timer for keymap flash save (0 = no save pending)
static uint64_t keymap_save_deadline = 0;
#define KEYMAP_SAVE_DELAY_MS 2000

// Debounce timer for macro flash save (0 = no save pending)
static uint64_t macro_save_deadline = 0;
static uint8_t macro_save_slot = 0;
#define MACRO_SAVE_DELAY_MS 2000

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
        current_burst_type = BURST_TYPE_LAYOUT;
        current_burst_state = BURST_SEND_START;
        current_layout_offset = 0;
    }
    else if (command == 0x03) {
        // Get Keymap: buffer[1] = layer index
        uint8_t layer = (bufsize > 1) ? buffer[1] : 0;
        if (layer >= MAX_LAYERS) layer = 0;
        keymap_burst_layer = layer;
        keymap_burst_chunk = 0;
        current_burst_type = BURST_TYPE_KEYMAP;
        current_burst_state = BURST_SEND_START;
    }
    else if (command == 0x04) {
        // Set Key: [0x04, layer, row, col, keycode_lo, keycode_hi]
        if (bufsize < 6) return;
        uint8_t layer = buffer[1];
        uint8_t row = buffer[2];
        uint8_t col = buffer[3];
        uint16_t kc = (uint16_t)buffer[4] | ((uint16_t)buffer[5] << 8);

        if (layer >= MAX_LAYERS) return;

        bool wrote = false;
        if (row == 0xFF) {
            // Encoder: col encodes encoder_index * 3 + action
            uint8_t enc_idx = col / 3;
            uint8_t action = col % 3;
            if (enc_idx < ENCODER_COUNT) {
                encoder_map[layer][enc_idx][action] = kc;
                wrote = true;
            }
        } else {
            if (row < MATRIX_ROWS && col < MATRIX_COLS) {
                keymap[layer][row][col] = kc;
                wrote = true;
            }
        }

        if (wrote) {
            keymap_save_deadline = time_us_64() + (uint64_t)KEYMAP_SAVE_DELAY_MS * 1000;
        }
    }
    else if (command == 0x05) {
        // Get Macro: buffer[1] = slot index
        uint8_t slot = (bufsize > 1) ? buffer[1] : 0;
        if (slot >= MACRO_MAX_SLOTS) return;
        macro_burst_slot = slot;
        macro_burst_chunk = 0;
        current_burst_type = BURST_TYPE_MACRO;
        current_burst_state = BURST_SEND_START;
    }
    else if (command == 0x06) {
        // Set Macro (multi-packet)
        if (bufsize < 4) return;
        uint8_t slot = buffer[1];
        if (slot >= MACRO_MAX_SLOTS) return;

        if (buffer[2] != 0xFF) {
            // Start packet: [0x06, slot, action_count, 0x00, data...]
            macro_rx_slot = slot;
            macro_rx_action_count = buffer[2];
            memset(macro_rx_buf, 0, sizeof(macro_rx_buf));
            uint8_t copy_len = (bufsize > 32) ? 28 : (bufsize - 4);
            memcpy(macro_rx_buf, &buffer[4], copy_len);

            uint16_t total_bytes = (uint16_t)macro_rx_action_count * 3 + 1;
            if (total_bytes <= copy_len) {
                // Fits in one packet — apply immediately to RAM
                macro_load_slot_ram(macro_rx_slot, macro_rx_buf, macro_rx_action_count);
                macro_save_slot = macro_rx_slot;
                macro_save_deadline = time_us_64() + (uint64_t)MACRO_SAVE_DELAY_MS * 1000;
            }
        } else {
            // Continue packet: [0x06, slot, 0xFF, offset, data...]
            uint8_t offset = buffer[3];
            uint8_t copy_len = (bufsize > 32) ? 28 : (bufsize - 4);
            if (offset + copy_len <= MACRO_SLOT_SIZE) {
                memcpy(&macro_rx_buf[offset], &buffer[4], copy_len);
            }

            uint16_t total_bytes = (uint16_t)macro_rx_action_count * 3 + 1;
            if (offset + copy_len >= total_bytes) {
                // All data received — apply to RAM, schedule flash
                macro_load_slot_ram(macro_rx_slot, macro_rx_buf, macro_rx_action_count);
                macro_save_slot = macro_rx_slot;
                macro_save_deadline = time_us_64() + (uint64_t)MACRO_SAVE_DELAY_MS * 1000;
            }
        }
    }
    else if (command == 0x07) {
        // Clear Macro: buffer[1] = slot index
        if (bufsize < 2) return;
        uint8_t slot = buffer[1];
        if (slot >= MACRO_MAX_SLOTS) return;
        macro_clear_slot(slot);
        macro_save_slot = slot;
        macro_save_deadline = time_us_64() + (uint64_t)MACRO_SAVE_DELAY_MS * 1000;
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
                uint16_t code = keymap[*current_layer][r][c];
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
                uint16_t code = keymap[current_layer][r][c];
                if (code == KC_TRNS) {
                    code = keymap[0][r][c];
                }

                if (code == KC_LAY_NEXT) continue;
                if (IS_MACRO_KEYCODE(code)) continue;

                if (code >= 0xE0 && code <= 0xE7) {
                    *modifier |= (1 << (code - 0xE0));
                }
                else if (code != 0 && *count < 6) {
                    keycode[*count] = (uint8_t)code;
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
            uint16_t enc_code = (delta > 0) ? encoder_map[current_layer][i][0] : encoder_map[current_layer][i][1];
            if (enc_code == KC_TRNS) {
                enc_code = (delta > 0) ? encoder_map[0][i][0] : encoder_map[0][i][1];
            }
            if (IS_MACRO_KEYCODE(enc_code)) {
                macro_trigger(MACRO_SLOT_FROM_KC(enc_code));
            } else if (enc_code != 0 && *count < 6) {
                keycode[*count] = (uint8_t)enc_code;
                (*count)++;
            }
        }

        if (encoder_get_click(i)) {
            *any_key_pressed = true;
            uint16_t click_code = encoder_map[current_layer][i][2];
            if (click_code == KC_TRNS) {
                click_code = encoder_map[0][i][2];
            }
            if (click_code == KC_LAY_NEXT) continue;
            if (IS_MACRO_KEYCODE(click_code)) {
                macro_trigger(MACRO_SLOT_FROM_KC(click_code));
                continue;
            }

            if (click_code >= 0xE0 && click_code <= 0xE7) {
                *modifier |= (1 << (click_code - 0xE0));
            }
            else if (click_code != 0 && *count < 6) {
                keycode[*count] = (uint8_t)click_code;
                (*count)++;
            }
        }
    }
#endif
}

// Scan matrix for any pressed macro keycode. Returns the keycode, or 0 if none.
uint16_t check_macro_trigger(uint8_t current_layer) {
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (matrix_is_on(r, c)) {
                uint16_t code = keymap[current_layer][r][c];
                if (code == KC_TRNS) code = keymap[0][r][c];
                if (IS_MACRO_KEYCODE(code)) return code;
            }
        }
    }
    return 0;
}

// --- KEYMAP BURST HELPERS ---

// Serialize one layer's keymap + encoder data into a flat buffer.
static uint16_t serialize_keymap_layer(uint8_t layer, uint8_t *buf) {
    uint16_t offset = 0;

    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            uint16_t kc = keymap[layer][r][c];
            buf[offset++] = (uint8_t)(kc & 0xFF);
            buf[offset++] = (uint8_t)(kc >> 8);
        }
    }

    for (int e = 0; e < ENCODER_COUNT; e++) {
        for (int a = 0; a < 3; a++) {
            uint16_t kc = encoder_map[layer][e][a];
            buf[offset++] = (uint8_t)(kc & 0xFF);
            buf[offset++] = (uint8_t)(kc >> 8);
        }
    }

    return offset;
}

// Total keymap data bytes per layer
#define KEYMAP_LAYER_BYTES ((MATRIX_ROWS * MATRIX_COLS + ENCODER_COUNT * 3) * 2)
// Payload per chunk (32 - 2 header bytes = 30)
#define KEYMAP_CHUNK_PAYLOAD 30
// Number of chunks needed
#define KEYMAP_CHUNK_COUNT ((KEYMAP_LAYER_BYTES + KEYMAP_CHUNK_PAYLOAD - 1) / KEYMAP_CHUNK_PAYLOAD)

// Scratch buffer for serialized keymap layer data
static uint8_t keymap_serial_buf[KEYMAP_LAYER_BYTES];

// --- MAIN LOOP ---

int main()
{
    stdio_init_all();
    matrix_init();
    tusb_init();
    macro_init();
    keymap_store_init();

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

    // Edge detection for macro key presses (prevent re-trigger while held)
    bool macro_key_held = false;

#if LAYOUT_ITEM_COUNT > 0
    uint8_t layout_data[] = PHYSICAL_LAYOUT_DATA;
    uint16_t total_layout_bytes = sizeof(layout_data);
#else
    uint16_t total_layout_bytes = 0;
#endif

    while (true) {
        tud_task();
        matrix_scan();

#if ENCODER_COUNT > 0
        encoder_update_clicks();
#endif

        // --- Debounced keymap flash save ---
        if (keymap_save_deadline != 0 && time_us_64() >= keymap_save_deadline) {
            keymap_store_save();
            keymap_save_deadline = 0;
        }

        // --- Debounced macro flash save ---
        if (macro_save_deadline != 0 && time_us_64() >= macro_save_deadline) {
            macro_save_slot_flash(macro_save_slot);
            macro_save_deadline = 0;
        }

        // --- 1. WEBHID CONFIGURATOR LOGIC (Interface 1) ---
        if (tud_hid_n_ready(1)) {

            if (has_metadata_response) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x01;
                report_buffer[1] = MATRIX_ROWS;
                report_buffer[2] = MATRIX_COLS;
                report_buffer[3] = MAX_LAYERS;
                report_buffer[4] = ENCODER_COUNT;

                // Append keyboard ID string: [id_len, id_bytes...]
                const char *keyboard_id = KEYBOARD_ID;
                uint8_t id_len = (uint8_t)strlen(keyboard_id);
                if (id_len > 26) id_len = 26; // Cap to fit 32-byte packet
                report_buffer[5] = id_len;
                memcpy(&report_buffer[6], keyboard_id, id_len);

                tud_hid_n_report(1, 0, report_buffer, 32);
                has_metadata_response = false;
            }
            else if (current_burst_state == BURST_SEND_START) {
                memset(report_buffer, 0, sizeof(report_buffer));
                report_buffer[0] = 0x12;

                if (current_burst_type == BURST_TYPE_LAYOUT) {
                    report_buffer[1] = total_layout_bytes / 5;
                } else if (current_burst_type == BURST_TYPE_KEYMAP) {
                    serialize_keymap_layer(keymap_burst_layer, keymap_serial_buf);
                    report_buffer[1] = KEYMAP_CHUNK_COUNT;
                } else {
                    // Macro burst
                    report_buffer[1] = MACRO_BURST_CHUNK_COUNT;
                }

                tud_hid_n_report(1, 0, report_buffer, 32);
                current_burst_state = BURST_SEND_CHUNKS;
            }
            else if (current_burst_state == BURST_SEND_CHUNKS) {
                memset(report_buffer, 0, sizeof(report_buffer));

                if (current_burst_type == BURST_TYPE_LAYOUT) {
                    report_buffer[0] = 0x02;
                    report_buffer[1] = (uint8_t)(current_layout_offset & 0xFF);

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
                } else if (current_burst_type == BURST_TYPE_KEYMAP) {
                    report_buffer[0] = 0x03;
                    uint16_t byte_offset = (uint16_t)keymap_burst_chunk * KEYMAP_CHUNK_PAYLOAD;
                    report_buffer[1] = (uint8_t)byte_offset;

                    uint16_t remaining = KEYMAP_LAYER_BYTES - byte_offset;
                    uint16_t to_copy = (remaining < KEYMAP_CHUNK_PAYLOAD) ? remaining : KEYMAP_CHUNK_PAYLOAD;
                    memcpy(&report_buffer[2], &keymap_serial_buf[byte_offset], to_copy);

                    tud_hid_n_report(1, 0, report_buffer, 32);
                    keymap_burst_chunk++;

                    if (keymap_burst_chunk >= KEYMAP_CHUNK_COUNT) {
                        current_burst_state = BURST_SEND_END;
                    }
                } else {
                    // Macro burst — send 256 bytes of macro_slot_t
                    report_buffer[0] = 0x05;
                    uint16_t byte_offset = (uint16_t)macro_burst_chunk * MACRO_BURST_PAYLOAD;
                    report_buffer[1] = (uint8_t)byte_offset;

                    const macro_slot_t *slot = macro_get_slot(macro_burst_slot);
                    const uint8_t *slot_bytes = (const uint8_t *)slot;
                    uint16_t remaining = MACRO_SLOT_SIZE - byte_offset;
                    uint16_t to_copy = (remaining < MACRO_BURST_PAYLOAD) ? remaining : MACRO_BURST_PAYLOAD;
                    memcpy(&report_buffer[2], &slot_bytes[byte_offset], to_copy);

                    tud_hid_n_report(1, 0, report_buffer, 32);
                    macro_burst_chunk++;

                    if (macro_burst_chunk >= MACRO_BURST_CHUNK_COUNT) {
                        current_burst_state = BURST_SEND_END;
                    }
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
            if (macro_is_playing()) {
                // Macro engine produces the HID report
                uint8_t macro_mod = 0;
                uint8_t macro_keys[6] = {0};
                if (macro_tick(&macro_mod, macro_keys)) {
                    tud_hid_keyboard_report(0, macro_mod, macro_keys);
                    previous_report_had_keys = true;
                    any_key_pressed = true;
                } else if (!macro_is_playing() && previous_report_had_keys) {
                    // Macro just finished — send empty release report
                    uint8_t empty[6] = {0};
                    tud_hid_keyboard_report(0, 0, empty);
                    previous_report_had_keys = false;
                }
            } else {
                // Regular key processing
                uint8_t keycode[6] = {0};
                uint8_t modifier = 0;
                int count = 0;

                process_layer_cycle(&current_layer, &layer_key_was_pressed, &any_key_pressed);

                // Check for macro triggers (edge-detected)
                uint16_t macro_kc = check_macro_trigger(current_layer);
                if (macro_kc != 0 && !macro_key_held) {
                    macro_trigger(MACRO_SLOT_FROM_KC(macro_kc));
                    macro_key_held = true;
                    any_key_pressed = true;
                } else if (macro_kc == 0) {
                    macro_key_held = false;
                }

                process_matrix_keys(current_layer, keycode, &modifier, &count);
                process_encoders(current_layer, keycode, &modifier, &count, &any_key_pressed);

                // Only send if not playing a macro that just started this tick
                if (!macro_is_playing()) {
                    bool current_report_has_keys = (count > 0 || modifier > 0);
                    if (current_report_has_keys || previous_report_had_keys) {
                        tud_hid_keyboard_report(0, modifier, keycode);
                    }
                    previous_report_had_keys = current_report_has_keys;
                }
            }
        }

#if ENABLE_OLED
        oled_task(current_layer, any_key_pressed);
#endif

    }
    return 0;
}
