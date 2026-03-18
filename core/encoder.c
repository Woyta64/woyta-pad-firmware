#include "pico/stdlib.h"
#include "generated_config.h"

#if ENCODER_COUNT > 0

static const uint8_t pad_a[] = ENCODER_PAD_A_PINS;
static const uint8_t pad_b[] = ENCODER_PAD_B_PINS;
static const uint8_t click_pins[] = ENCODER_CLICK_PINS;

static uint8_t state[ENCODER_COUNT];
static int32_t position[ENCODER_COUNT]; // Stores net movement (+1, -1)


void encoder_init(void) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        // Initialize Pad A
        gpio_init(pad_a[i]);
        gpio_set_dir(pad_a[i], GPIO_IN);
        gpio_pull_up(pad_a[i]);

        // Initialize Pad B
        gpio_init(pad_b[i]);
        gpio_set_dir(pad_b[i], GPIO_IN);
        gpio_pull_up(pad_b[i]);

        // Initialize Click Pins
        gpio_init(click_pins[i]);
        gpio_set_dir(click_pins[i], GPIO_IN);
        gpio_pull_up(click_pins[i]);

        // Initial state
        uint8_t a = gpio_get(pad_a[i]);
        uint8_t b = gpio_get(pad_b[i]);
        state[i] = (a << 1) | b;
    }
}

void encoder_read(void) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        uint8_t a = gpio_get(pad_a[i]);
        uint8_t b = gpio_get(pad_b[i]);
        uint8_t new_state = (a << 1) | b;

        // Determine transition
        // This is a standard quadrature state machine
        if (state[i] != new_state) {
            // Check for valid moves to filter noise
            if ((state[i] == 0b00 && new_state == 0b01) ||
                (state[i] == 0b01 && new_state == 0b11) ||
                (state[i] == 0b11 && new_state == 0b10) ||
                (state[i] == 0b10 && new_state == 0b00)) {
                position[i]++; // CW
                }
            else if ((state[i] == 0b00 && new_state == 0b10) ||
                     (state[i] == 0b10 && new_state == 0b11) ||
                     (state[i] == 0b11 && new_state == 0b01) ||
                     (state[i] == 0b01 && new_state == 0b00)) {
                position[i]--; // CCW
                     }
            state[i] = new_state;
        }
    }
}

// Get and clear the delta (Change since last check)
int32_t encoder_get_delta(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;

    // Divide by resolution (usually 4 steps per click)
    int32_t val = position[index];

    // Hysteresis / Resolution handling
    if (val >= 4 || val <= -4) {
        position[index] = 0; // Reset
        return (val > 0) ? 1 : -1;
    }
    return 0;
}

bool encoder_get_click(uint8_t index) {
    if (index >= ENCODER_COUNT) return false;

    // Encoder buttons connect to ground when pressed (Active Low)
    // So if gpio_get reads 0, the button is pressed (!0 = true)
    return !gpio_get(click_pins[index]);
}

#endif