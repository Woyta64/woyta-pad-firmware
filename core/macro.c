#include "macro.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/regs/addressmap.h"

// Flash layout: 9 sectors at end of 2 MB flash
// Sector 0: Header (magic + version)
// Sectors 1-8: 128 macro slots × 256 bytes each
#define MACRO_FLASH_OFFSET   (2 * 1024 * 1024 - 9 * FLASH_SECTOR_SIZE)
#define MACRO_HEADER_OFFSET  MACRO_FLASH_OFFSET
#define MACRO_DATA_OFFSET    (MACRO_FLASH_OFFSET + FLASH_SECTOR_SIZE)

#define MACRO_MAGIC_0  0x4D  // 'M'
#define MACRO_MAGIC_1  0x43  // 'C'
#define MACRO_VERSION  0x01


// --- RAM storage ---
static macro_slot_t macros[MACRO_MAX_SLOTS];

// --- Engine state ---
typedef enum {
    MACRO_STATE_IDLE,
    MACRO_STATE_PLAYING,
    MACRO_STATE_TAP_RELEASE,
    MACRO_STATE_DELAY
} macro_engine_state_t;

typedef struct {
    macro_engine_state_t state;
    uint8_t current_slot;
    uint8_t data_offset;
    uint8_t modifier;
    uint8_t keys[6];
    uint8_t key_count;
    // Saved state for TAP release (restores pre-TAP held keys)
    uint8_t tap_saved_modifier;
    uint8_t tap_saved_keys[6];
    uint8_t tap_saved_key_count;
    uint64_t delay_until;
} macro_engine_t;

static macro_engine_t engine;

// --- Helpers ---

static void engine_add_key(uint8_t keycode) {
    if (keycode == 0 || engine.key_count >= 6) return;
    engine.keys[engine.key_count++] = keycode;
}

static void engine_remove_key(uint8_t keycode) {
    for (int i = 0; i < engine.key_count; i++) {
        if (engine.keys[i] == keycode) {
            for (int j = i; j < engine.key_count - 1; j++) {
                engine.keys[j] = engine.keys[j + 1];
            }
            engine.keys[--engine.key_count] = 0;
            return;
        }
    }
}

static void engine_fill_report(uint8_t *modifier, uint8_t *keycodes) {
    *modifier = engine.modifier;
    memcpy(keycodes, engine.keys, 6);
}

// --- Public API ---

void macro_init(void) {
    memset(&engine, 0, sizeof(engine));
    engine.state = MACRO_STATE_IDLE;

    const uint8_t *flash_header = (const uint8_t *)(XIP_BASE + MACRO_HEADER_OFFSET);

    if (flash_header[0] == MACRO_MAGIC_0 &&
        flash_header[1] == MACRO_MAGIC_1 &&
        flash_header[2] == MACRO_VERSION) {
        // Valid header — load all macro slots from flash
        const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + MACRO_DATA_OFFSET);
        memcpy(macros, flash_data, sizeof(macros));
    } else {
        // No valid data — all slots empty
        memset(macros, 0xFF, sizeof(macros));
    }
}

bool macro_trigger(uint8_t slot) {
    if (engine.state != MACRO_STATE_IDLE) return false;
    if (slot >= MACRO_MAX_SLOTS) return false;
    if (macros[slot].status != 0x01) return false;

    engine.current_slot = slot;
    engine.data_offset = 0;
    engine.modifier = 0;
    memset(engine.keys, 0, sizeof(engine.keys));
    engine.key_count = 0;
    engine.state = MACRO_STATE_PLAYING;

    return true;
}

bool macro_tick(uint8_t *modifier, uint8_t *keycodes) {
    if (engine.state == MACRO_STATE_IDLE) return false;

    macro_slot_t *slot = &macros[engine.current_slot];

    // Waiting for delay to elapse
    if (engine.state == MACRO_STATE_DELAY) {
        if (time_us_64() < engine.delay_until) return false;
        engine.state = MACRO_STATE_PLAYING;
    }

    // Send the release half of a TAP
    if (engine.state == MACRO_STATE_TAP_RELEASE) {
        engine.modifier = engine.tap_saved_modifier;
        engine.key_count = engine.tap_saved_key_count;
        memcpy(engine.keys, engine.tap_saved_keys, 6);

        engine_fill_report(modifier, keycodes);
        engine.state = MACRO_STATE_PLAYING;
        return true;
    }

    // Process next action
    if (engine.data_offset >= MACRO_DATA_SIZE) {
        engine.state = MACRO_STATE_IDLE;
        return false;
    }

    uint8_t action_type = slot->data[engine.data_offset];

    if (action_type == MACRO_END) {
        // Release any held keys before stopping
        if (engine.modifier != 0 || engine.key_count > 0) {
            engine.modifier = 0;
            memset(engine.keys, 0, sizeof(engine.keys));
            engine.key_count = 0;
            engine_fill_report(modifier, keycodes);
            engine.state = MACRO_STATE_IDLE;
            return true;
        }
        engine.state = MACRO_STATE_IDLE;
        return false;
    }

    // All non-END actions are 3 bytes
    if (engine.data_offset + 2 >= MACRO_DATA_SIZE) {
        engine.state = MACRO_STATE_IDLE;
        return false;
    }

    uint8_t byte1 = slot->data[engine.data_offset + 1];
    uint8_t byte2 = slot->data[engine.data_offset + 2];
    engine.data_offset += 3;

    switch (action_type) {
        case MACRO_TAP:
            // Save current state so we can restore after release
            engine.tap_saved_modifier = engine.modifier;
            engine.tap_saved_key_count = engine.key_count;
            memcpy(engine.tap_saved_keys, engine.keys, 6);
            // Add TAP modifier + key on top of current state
            engine.modifier |= byte1;
            if (byte2 != 0) engine_add_key(byte2);
            engine_fill_report(modifier, keycodes);
            engine.state = MACRO_STATE_TAP_RELEASE;
            return true;

        case MACRO_DOWN:
            engine.modifier |= byte1;
            if (byte2 != 0) engine_add_key(byte2);
            engine_fill_report(modifier, keycodes);
            return true;

        case MACRO_UP:
            engine.modifier &= ~byte1;
            if (byte2 != 0) engine_remove_key(byte2);
            engine_fill_report(modifier, keycodes);
            return true;

        case MACRO_DELAY: {
            uint16_t delay_ms = ((uint16_t)byte1 << 8) | byte2;
            engine.delay_until = time_us_64() + (uint64_t)delay_ms * 1000;
            engine.state = MACRO_STATE_DELAY;
            return false;
        }

        default:
            engine.state = MACRO_STATE_IDLE;
            return false;
    }
}

bool macro_is_playing(void) {
    return engine.state != MACRO_STATE_IDLE;
}

void macro_write_slot(uint8_t slot, const uint8_t *data, uint8_t action_count) {
    if (slot >= MACRO_MAX_SLOTS) return;

    // Update RAM
    macros[slot].status = 0x01;
    macros[slot].action_count = action_count;
    memset(macros[slot].data, 0, MACRO_DATA_SIZE);
    uint16_t copy_len = (uint16_t)action_count * 3 + 1;
    if (copy_len > MACRO_DATA_SIZE) copy_len = MACRO_DATA_SIZE;
    memcpy(macros[slot].data, data, copy_len);

    // Ensure header sector is initialized
    const uint8_t *flash_header = (const uint8_t *)(XIP_BASE + MACRO_HEADER_OFFSET);
    if (flash_header[0] != MACRO_MAGIC_0 ||
        flash_header[1] != MACRO_MAGIC_1 ||
        flash_header[2] != MACRO_VERSION) {
        uint8_t header[FLASH_PAGE_SIZE];
        memset(header, 0xFF, sizeof(header));
        header[0] = MACRO_MAGIC_0;
        header[1] = MACRO_MAGIC_1;
        header[2] = MACRO_VERSION;
        header[3] = MACRO_MAX_SLOTS;

        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(MACRO_HEADER_OFFSET, FLASH_SECTOR_SIZE);
        flash_range_program(MACRO_HEADER_OFFSET, header, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
    }

    // Write the sector containing this slot (16 slots per sector)
    uint8_t sector_idx = slot / 16;
    uint32_t sector_flash_offset = MACRO_DATA_OFFSET + sector_idx * FLASH_SECTOR_SIZE;
    uint8_t base_slot = sector_idx * 16;

    // Build full sector from RAM
    static uint8_t sector_buffer[FLASH_SECTOR_SIZE];
    memcpy(sector_buffer, &macros[base_slot], 16 * MACRO_SLOT_SIZE);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(sector_flash_offset, FLASH_SECTOR_SIZE);
    flash_range_program(sector_flash_offset, sector_buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

void macro_load_slot_ram(uint8_t slot, const uint8_t *data, uint8_t action_count) {
    if (slot >= MACRO_MAX_SLOTS) return;
    macros[slot].status = 0x01;
    macros[slot].action_count = action_count;
    memset(macros[slot].data, 0, MACRO_DATA_SIZE);
    uint16_t copy_len = (uint16_t)action_count * 3 + 1;
    if (copy_len > MACRO_DATA_SIZE) copy_len = MACRO_DATA_SIZE;
    memcpy(macros[slot].data, data, copy_len);
}

const macro_slot_t* macro_get_slot(uint8_t slot) {
    if (slot >= MACRO_MAX_SLOTS) return NULL;
    return &macros[slot];
}

void macro_save_slot_flash(uint8_t slot) {
    if (slot >= MACRO_MAX_SLOTS) return;

    // Ensure header sector is initialized
    const uint8_t *flash_header = (const uint8_t *)(XIP_BASE + MACRO_HEADER_OFFSET);
    if (flash_header[0] != MACRO_MAGIC_0 ||
        flash_header[1] != MACRO_MAGIC_1 ||
        flash_header[2] != MACRO_VERSION) {
        uint8_t header[FLASH_PAGE_SIZE];
        memset(header, 0xFF, sizeof(header));
        header[0] = MACRO_MAGIC_0;
        header[1] = MACRO_MAGIC_1;
        header[2] = MACRO_VERSION;
        header[3] = MACRO_MAX_SLOTS;

        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(MACRO_HEADER_OFFSET, FLASH_SECTOR_SIZE);
        flash_range_program(MACRO_HEADER_OFFSET, header, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
    }

    // Write the sector containing this slot (16 slots per sector)
    uint8_t sector_idx = slot / 16;
    uint32_t sector_flash_offset = MACRO_DATA_OFFSET + sector_idx * FLASH_SECTOR_SIZE;
    uint8_t base_slot = sector_idx * 16;

    static uint8_t sector_buffer[FLASH_SECTOR_SIZE];
    memcpy(sector_buffer, &macros[base_slot], 16 * MACRO_SLOT_SIZE);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(sector_flash_offset, FLASH_SECTOR_SIZE);
    flash_range_program(sector_flash_offset, sector_buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

void macro_clear_slot(uint8_t slot) {
    if (slot >= MACRO_MAX_SLOTS) return;
    macros[slot].status = 0xFF;
    macros[slot].action_count = 0;
    memset(macros[slot].data, 0, MACRO_DATA_SIZE);
}
