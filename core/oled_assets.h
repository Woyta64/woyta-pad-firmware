#pragma once

#include "oled_types.h"
#include "default_bitmaps.h"    // built-in fallbacks, always compiled in
#include "generated_bitmaps.h"  // user-provided assets; defines HAS_BITMAP_* when present

// Optional keyboard-specific overrides (just #define constants, no structs needed)
#if __has_include("assets.h")
#include "assets.h"
#endif

// --- DEFAULT TIMINGS (override in keyboard's assets.h if needed) ---
#ifndef OLED_IDLE_TIMEOUT_MS
#define OLED_IDLE_TIMEOUT_MS  10000
#endif
#ifndef OLED_SLEEP_TIMEOUT_MS
#define OLED_SLEEP_TIMEOUT_MS 60000
#endif
#ifndef OLED_BOOT_DURATION_MS
#define OLED_BOOT_DURATION_MS 3000
#endif
#ifndef OLED_IDLE_FRAME_MS
#define OLED_IDLE_FRAME_MS    150
#endif

// --- BOOT (optional image, text fallback handled in oled.cpp) ---
#ifdef HAS_BITMAP_BOOT
static const oled_anim_t anim_boot = {
    .bitmap = bitmap_boot, .frames = 1, .frame_ms = OLED_BOOT_DURATION_MS
};
#endif

// --- IDLE ---
static const oled_anim_t anim_idle = {
#ifdef HAS_BITMAP_IDLE
    .bitmap = bitmap_idle,
#else
    .bitmap = default_bitmap_idle,
#endif
    .frames = 8, .frame_ms = OLED_IDLE_FRAME_MS
};

// --- LAYER IMAGES ---
static const oled_anim_t anim_layers[4] = {
    {
#ifdef HAS_BITMAP_LAYER1
        .bitmap = bitmap_layer1,
#else
        .bitmap = default_bitmap_layer1,
#endif
        .frames = 1, .frame_ms = 0
    },
    {
#ifdef HAS_BITMAP_LAYER2
        .bitmap = bitmap_layer2,
#else
        .bitmap = default_bitmap_layer2,
#endif
        .frames = 1, .frame_ms = 0
    },
    {
#ifdef HAS_BITMAP_LAYER3
        .bitmap = bitmap_layer3,
#else
        .bitmap = default_bitmap_layer3,
#endif
        .frames = 1, .frame_ms = 0
    },
    {
#ifdef HAS_BITMAP_LAYER4
        .bitmap = bitmap_layer4,
#else
        .bitmap = default_bitmap_layer4,
#endif
        .frames = 1, .frame_ms = 0
    },
};

#define MAX_LAYER_IMAGES 4
static const oled_anim_t* layer_images[MAX_LAYER_IMAGES] = {
    &anim_layers[0], &anim_layers[1], &anim_layers[2], &anim_layers[3]
};

// --- STATE LOOKUP ---
static const oled_anim_t* anim_lookup[OLED_STATE_COUNT] = {
#ifdef HAS_BITMAP_BOOT
    [OLED_STATE_BOOT] = &anim_boot,
#else
    [OLED_STATE_BOOT] = NULL,   // oled.cpp renders keyboard name as text
#endif
    [OLED_STATE_ACTIVE] = NULL, // oled.cpp renders layer image from layer_images[]
    [OLED_STATE_IDLE]   = &anim_idle,
    [OLED_STATE_SLEEP]  = NULL
};
