#include "keymap_store.h"
#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/regs/addressmap.h"
#include "generated_config.h"
#include "keycodes.h"

extern uint16_t keymap[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];
#if ENCODER_COUNT > 0
extern uint16_t encoder_map[MAX_LAYERS][ENCODER_COUNT][3];
#endif

// Flash layout: 1 sector reserved for keymap, right before macro's 9 sectors
// Macro uses last 9 sectors, keymap uses the 10th-from-end sector
#define KEYMAP_FLASH_OFFSET  (2 * 1024 * 1024 - 10 * FLASH_SECTOR_SIZE)

#define KEYMAP_MAGIC_0  0x4B  // 'K'
#define KEYMAP_MAGIC_1  0x4D  // 'M'
#define KEYMAP_VERSION  0x01

// Data layout in flash page:
// Bytes 0-2: magic + version
// Byte 3: reserved
// Bytes 4+: keymap data then encoder data

void keymap_store_init(void) {
    const uint8_t *flash = (const uint8_t *)(XIP_BASE + KEYMAP_FLASH_OFFSET);

    if (flash[0] == KEYMAP_MAGIC_0 &&
        flash[1] == KEYMAP_MAGIC_1 &&
        flash[2] == KEYMAP_VERSION) {
        // Valid data — load into RAM arrays
        const uint8_t *src = flash + 4;
        memcpy(keymap, src, sizeof(uint16_t) * MAX_LAYERS * MATRIX_ROWS * MATRIX_COLS);
#if ENCODER_COUNT > 0
        src += sizeof(uint16_t) * MAX_LAYERS * MATRIX_ROWS * MATRIX_COLS;
        memcpy(encoder_map, src, sizeof(uint16_t) * MAX_LAYERS * ENCODER_COUNT * 3);
#endif
    }
    // Otherwise keep the compiled-in defaults
}

void keymap_store_save(void) {
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));

    page[0] = KEYMAP_MAGIC_0;
    page[1] = KEYMAP_MAGIC_1;
    page[2] = KEYMAP_VERSION;
    page[3] = 0x00;

    uint8_t *dst = page + 4;
    memcpy(dst, keymap, sizeof(uint16_t) * MAX_LAYERS * MATRIX_ROWS * MATRIX_COLS);
#if ENCODER_COUNT > 0
    dst += sizeof(uint16_t) * MAX_LAYERS * MATRIX_ROWS * MATRIX_COLS;
    memcpy(dst, encoder_map, sizeof(uint16_t) * MAX_LAYERS * ENCODER_COUNT * 3);
#endif

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(KEYMAP_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(KEYMAP_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}
