#include <stdint.h>
#include "keycodes.h"
#include "generated_config.h"

// MATRIX KEYMAP
// Structure: [Layer][Row][Column]
uint16_t keymap[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS] = {

    // Layer 0: Numpad
    [0] = {
        { KC_7, KC_8, KC_9 },
        { KC_4, KC_5, KC_6 },
        { KC_1, KC_2, KC_3 }
    },

    // Layer 1: Media
    [1] = {
        { KC_MEDIA_PREVIOUSSONG, KC_MEDIA_PLAYPAUSE, KC_MEDIA_NEXTSONG },
        { KC_MEDIA_VOLUMEDOWN,   KC_MEDIA_MUTE,      KC_MEDIA_VOLUMEUP },
        { KC_NONE,               KC_NONE,             KC_NONE }
    }
};

// ENCODER MAP
// Structure: [Layer][Encoder Index][CW, CCW, Click]
uint16_t encoder_map[MAX_LAYERS][ENCODER_COUNT][3] = {

    [0] = {
        { KC_MEDIA_VOLUMEUP, KC_MEDIA_VOLUMEDOWN, KC_MEDIA_MUTE }
    },

    [1] = {
        { KC_RIGHT, KC_LEFT, KC_LAY_NEXT }
    }
};
