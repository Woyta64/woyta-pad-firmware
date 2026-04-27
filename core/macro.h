#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MACRO_MAX_SLOTS   128
#define MACRO_SLOT_SIZE   256
#define MACRO_DATA_SIZE   253

// Action type opcodes
#define MACRO_END    0x00
#define MACRO_TAP    0x01
#define MACRO_DOWN   0x02
#define MACRO_UP     0x03
#define MACRO_DELAY  0x04

typedef struct {
    uint8_t status;       // 0xFF = empty, 0x01 = active
    uint8_t action_count;
    uint8_t data[MACRO_DATA_SIZE];
    uint8_t _reserved;
} macro_slot_t;

// Initialize macro system (load from flash into RAM)
void macro_init(void);

// Start playing a macro slot. Returns true if started, false if busy or invalid.
bool macro_trigger(uint8_t slot);

// Advance the macro engine one step. Returns true if a HID report should be sent.
// Fills modifier and keycodes for the report.
bool macro_tick(uint8_t *modifier, uint8_t *keycodes);

// Returns true if the macro engine is currently playing
bool macro_is_playing(void);

// Write a single macro slot to flash
void macro_write_slot(uint8_t slot, const uint8_t *data, uint8_t action_count);

// Load a macro into RAM only (no flash write) — useful for testing
void macro_load_slot_ram(uint8_t slot, const uint8_t *data, uint8_t action_count);

// Read-only access to a macro slot (NULL if slot out of range)
const macro_slot_t* macro_get_slot(uint8_t slot);

// Save a single slot from RAM to flash (deferred write target)
void macro_save_slot_flash(uint8_t slot);

// Clear a macro slot in RAM (set status=0xFF, zero data)
void macro_clear_slot(uint8_t slot);
