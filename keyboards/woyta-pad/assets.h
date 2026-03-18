#pragma once

#include "oled_types.h"
#include "generated_bitmaps.h"

// --- TIMING CONFIG ---
#define OLED_IDLE_TIMEOUT_MS  10000
#define OLED_SLEEP_TIMEOUT_MS 60000

// --- STATE IMAGE DEFINITION ---
const oled_anim_t anim_boot = {
	.bitmap   = bitmap_boot, // This variable comes from generated_bitmaps.h
	.frames   = 1,           // boot.png is a single static image
	.frame_ms = 1000         // Show for 1 second
};

const oled_anim_t anim_idle = {
	.bitmap   = bitmap_idle, // This comes from generated_bitmaps.h
	.frames   = 3,           // idle.png is 128x64 (2 frames stacked)
	.frame_ms = 500          // Switch frames every 500ms
};

// --- STATE MAPPING ---
// The engine simply looks up the state index to find the animation
const oled_anim_t* anim_lookup[OLED_STATE_COUNT] = {
	[OLED_STATE_BOOT]   = &anim_boot,
	[OLED_STATE_ACTIVE] = NULL,       // Active is text-based (code handles this)
	[OLED_STATE_IDLE]   = &anim_idle,
	[OLED_STATE_SLEEP]  = NULL        // Sleep is blank
};