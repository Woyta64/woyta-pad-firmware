#pragma once
#include <stdint.h>
#include <stdbool.h>

void encoder_init(void);
void encoder_read(void);
int32_t encoder_get_delta(uint8_t index);
void encoder_update_clicks(void);
bool encoder_get_click(uint8_t index);
