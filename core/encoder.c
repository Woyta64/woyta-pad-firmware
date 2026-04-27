#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "generated_config.h"

#if ENCODER_COUNT > 0

static const uint8_t pad_a[] = ENCODER_PAD_A_PINS;
static const uint8_t pad_b[] = ENCODER_PAD_B_PINS;
static const uint8_t click_pins[] = ENCODER_CLICK_PINS;

static uint8_t state[ENCODER_COUNT];
static volatile int32_t position[ENCODER_COUNT];

// Click edge detection (written + read from main loop only)
static bool click_prev[ENCODER_COUNT];
static bool click_pressed[ENCODER_COUNT];

void encoder_init(void) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        gpio_init(pad_a[i]);
        gpio_set_dir(pad_a[i], GPIO_IN);
        gpio_pull_up(pad_a[i]);

        gpio_init(pad_b[i]);
        gpio_set_dir(pad_b[i], GPIO_IN);
        gpio_pull_up(pad_b[i]);

        gpio_init(click_pins[i]);
        gpio_set_dir(click_pins[i], GPIO_IN);
        gpio_pull_up(click_pins[i]);

        uint8_t a = gpio_get(pad_a[i]);
        uint8_t b = gpio_get(pad_b[i]);
        state[i] = (a << 1) | b;
    }
}

// Called from timer ISR
void encoder_read(void) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        uint8_t a = gpio_get(pad_a[i]);
        uint8_t b = gpio_get(pad_b[i]);
        uint8_t new_state = (a << 1) | b;

        if (state[i] != new_state) {
            if ((state[i] == 0b00 && new_state == 0b01) ||
                (state[i] == 0b01 && new_state == 0b11) ||
                (state[i] == 0b11 && new_state == 0b10) ||
                (state[i] == 0b10 && new_state == 0b00)) {
                position[i]++;
            } else if ((state[i] == 0b00 && new_state == 0b10) ||
                       (state[i] == 0b10 && new_state == 0b11) ||
                       (state[i] == 0b11 && new_state == 0b01) ||
                       (state[i] == 0b01 && new_state == 0b00)) {
                position[i]--;
            }
            state[i] = new_state;
        }
    }
}

int32_t encoder_get_delta(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;

    // Atomic swap to avoid race with timer ISR
    int32_t val = position[index];
    if (val >= 4 || val <= -4) {
        uint32_t ints = save_and_disable_interrupts();
        val = position[index];
        position[index] = 0;
        restore_interrupts(ints);
        return (val > 0) ? 1 : -1;
    }
    return 0;
}

// Call from main loop each tick to update edge detection
void encoder_update_clicks(void) {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        bool raw = !gpio_get(click_pins[i]); // active low
        click_pressed[i] = raw && !click_prev[i]; // rising edge only
        click_prev[i] = raw;
    }
}

bool encoder_get_click(uint8_t index) {
    if (index >= ENCODER_COUNT) return false;
    return click_pressed[index];
}

#endif
