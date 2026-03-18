#pragma once
#include <stdint.h>

// Represents one animation sequence
typedef struct {
    const uint8_t* bitmap; // Pointer to raw byte array (GLCD or Vertical format)
    uint8_t  frames;       // How many frames in this strip
    uint16_t frame_ms;     // ms per frame (speed)
} oled_anim_t;

// State Definitions
typedef enum {
    OLED_STATE_BOOT,
    OLED_STATE_ACTIVE,
    OLED_STATE_IDLE,
    OLED_STATE_SLEEP,
    BUTTON_CLICK,
    LAYER_1,
    LAYER_2,
    LAYER_3,
    LAYER_4,
    LAYER_5,
    LAYER_6,
    LAYER_7,
    LAYER_8,
    OLED_STATE_COUNT // Used to size array of states
} oled_state_t;