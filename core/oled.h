#pragma once
#include <stdint.h>
#include <stdbool.h>

// Initializes I2C hardware and the Display library
void oled_init_wrapper(void);

// Main Engine Task
// layer: The current keyboard layer (0, 1, etc.)
// key_pressed: true if a key was pressed this frame (resets sleep timer)
void oled_task(uint8_t layer, bool key_pressed);