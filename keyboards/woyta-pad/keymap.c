#include <stdint.h>
#include "keycodes.h"
#include "generated_config.h"

#define LAYERS 4

// MATRIX KEYMAP
// Structure: [Layer][Row][Column]
const uint8_t keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS] = {

    [0] = {
        { KC_0,  KC_1, KC_2, KC_3, KC_LAY_NEXT },
        { KC_5,  KC_6, KC_7, KC_8, KC_9 },
        { KC_A,  KC_Z, KC_X, KC_C, KC_V },
        { KC_B,  KC_C, KC_SPACE, KC_ENTER, KC_BACKSPACE }
    },

    [1] = {
        { KC_TRNS, KC_7, KC_8, KC_9, KC_TRNS },
        { KC_TRNS, KC_4, KC_5, KC_6, KC_KPMINUS },
        { KC_TRNS, KC_1, KC_2, KC_3, KC_KPASTERISK },
        { KC_TRNS, KC_TRNS, KC_0, KC_KPDOT, KC_KPSLASH }
    },

    [2] = {
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_TRNS},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE}
    },

    [3] = {
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_TRNS},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE},
        { KC_NONE, KC_NONE, KC_NONE, KC_NONE, KC_NONE}
    },

};
