#pragma once

#include <stdbool.h>

// Initialize keymap store: load from flash into RAM if valid
void keymap_store_init(void);

// Save current keymap + encoder_map arrays to flash
void keymap_store_save(void);
