#pragma once

// --- CUSTOM WOYTA-PAD KEYCODES ---
#define KC_TRNS 0x00      // Transparent: Fall back to the key on Layer 0
#define KC_LAY_NEXT 0xF4  // Cycle to next layer

/**
 * Modifier masks - used for the first byte in the HID report.
 * NOTE: The second byte in the report is reserved, 0x00
 */
#define KC_MOD_LCTRL  0x01
#define KC_MOD_LSHIFT 0x02
#define KC_MOD_LALT   0x04
#define KC_MOD_LMETA  0x08
#define KC_MOD_RCTRL  0x10
#define KC_MOD_RSHIFT 0x20
#define KC_MOD_RALT   0x40
#define KC_MOD_RMETA  0x80

/**
 * Scan codes - last N slots in the HID report (usually 6).
 * 0x00 if no key pressed.
 * 
 * If more than N keys are pressed, the HID reports 
 * KC_ERR_OVF in all slots to indicate this condition.
 */

#define KC_NONE 0x00 // No key pressed
#define KC_ERR_OVF 0x01 //  Keyboard Error Roll Over - used for all slots if too many keys are pressed ("Phantom key")
// 0x02 //  Keyboard POST Fail
// 0x03 //  Keyboard Error Undefined
#define KC_A 0x04 // Keyboard a and A
#define KC_B 0x05 // Keyboard b and B
#define KC_C 0x06 // Keyboard c and C
#define KC_D 0x07 // Keyboard d and D
#define KC_E 0x08 // Keyboard e and E
#define KC_F 0x09 // Keyboard f and F
#define KC_G 0x0a // Keyboard g and G
#define KC_H 0x0b // Keyboard h and H
#define KC_I 0x0c // Keyboard i and I
#define KC_J 0x0d // Keyboard j and J
#define KC_K 0x0e // Keyboard k and K
#define KC_L 0x0f // Keyboard l and L
#define KC_M 0x10 // Keyboard m and M
#define KC_N 0x11 // Keyboard n and N
#define KC_O 0x12 // Keyboard o and O
#define KC_P 0x13 // Keyboard p and P
#define KC_Q 0x14 // Keyboard q and Q
#define KC_R 0x15 // Keyboard r and R
#define KC_S 0x16 // Keyboard s and S
#define KC_T 0x17 // Keyboard t and T
#define KC_U 0x18 // Keyboard u and U
#define KC_V 0x19 // Keyboard v and V
#define KC_W 0x1a // Keyboard w and W
#define KC_X 0x1b // Keyboard x and X
#define KC_Y 0x1c // Keyboard y and Y
#define KC_Z 0x1d // Keyboard z and Z

#define KC_1 0x1e // Keyboard 1 and !
#define KC_2 0x1f // Keyboard 2 and @
#define KC_3 0x20 // Keyboard 3 and #
#define KC_4 0x21 // Keyboard 4 and $
#define KC_5 0x22 // Keyboard 5 and %
#define KC_6 0x23 // Keyboard 6 and ^
#define KC_7 0x24 // Keyboard 7 and &
#define KC_8 0x25 // Keyboard 8 and *
#define KC_9 0x26 // Keyboard 9 and (
#define KC_0 0x27 // Keyboard 0 and )

#define KC_ENTER 0x28 // Keyboard Return (ENTER)
#define KC_ESC 0x29 // Keyboard ESCAPE
#define KC_BACKSPACE 0x2a // Keyboard DELETE (Backspace)
#define KC_TAB 0x2b // Keyboard Tab
#define KC_SPACE 0x2c // Keyboard Spacebar
#define KC_MINUS 0x2d // Keyboard - and _
#define KC_EQUAL 0x2e // Keyboard = and +
#define KC_LEFTBRACE 0x2f // Keyboard [ and {
#define KC_RIGHTBRACE 0x30 // Keyboard ] and }
#define KC_BACKSLASH 0x31 // Keyboard \ and |
#define KC_HASHTILDE 0x32 // Keyboard Non-US # and ~
#define KC_SEMICOLON 0x33 // Keyboard ; and :
#define KC_APOSTROPHE 0x34 // Keyboard ' and "
#define KC_GRAVE 0x35 // Keyboard ` and ~
#define KC_COMMA 0x36 // Keyboard , and <
#define KC_DOT 0x37 // Keyboard . and >
#define KC_SLASH 0x38 // Keyboard / and ?
#define KC_CAPSLOCK 0x39 // Keyboard Caps Lock

#define KC_F1 0x3a // Keyboard F1
#define KC_F2 0x3b // Keyboard F2
#define KC_F3 0x3c // Keyboard F3
#define KC_F4 0x3d // Keyboard F4
#define KC_F5 0x3e // Keyboard F5
#define KC_F6 0x3f // Keyboard F6
#define KC_F7 0x40 // Keyboard F7
#define KC_F8 0x41 // Keyboard F8
#define KC_F9 0x42 // Keyboard F9
#define KC_F10 0x43 // Keyboard F10
#define KC_F11 0x44 // Keyboard F11
#define KC_F12 0x45 // Keyboard F12

#define KC_SYSRQ 0x46 // Keyboard Print Screen
#define KC_SCROLLLOCK 0x47 // Keyboard Scroll Lock
#define KC_PAUSE 0x48 // Keyboard Pause
#define KC_INSERT 0x49 // Keyboard Insert
#define KC_HOME 0x4a // Keyboard Home
#define KC_PAGEUP 0x4b // Keyboard Page Up
#define KC_DELETE 0x4c // Keyboard Delete Forward
#define KC_END 0x4d // Keyboard End
#define KC_PAGEDOWN 0x4e // Keyboard Page Down
#define KC_RIGHT 0x4f // Keyboard Right Arrow
#define KC_LEFT 0x50 // Keyboard Left Arrow
#define KC_DOWN 0x51 // Keyboard Down Arrow
#define KC_UP 0x52 // Keyboard Up Arrow

#define KC_NUMLOCK 0x53 // Keyboard Num Lock and Clear
#define KC_KPSLASH 0x54 // Keypad /
#define KC_KPASTERISK 0x55 // Keypad *
#define KC_KPMINUS 0x56 // Keypad -
#define KC_KPPLUS 0x57 // Keypad +
#define KC_KPENTER 0x58 // Keypad ENTER
#define KC_KP1 0x59 // Keypad 1 and End
#define KC_KP2 0x5a // Keypad 2 and Down Arrow
#define KC_KP3 0x5b // Keypad 3 and PageDn
#define KC_KP4 0x5c // Keypad 4 and Left Arrow
#define KC_KP5 0x5d // Keypad 5
#define KC_KP6 0x5e // Keypad 6 and Right Arrow
#define KC_KP7 0x5f // Keypad 7 and Home
#define KC_KP8 0x60 // Keypad 8 and Up Arrow
#define KC_KP9 0x61 // Keypad 9 and Page Up
#define KC_KP0 0x62 // Keypad 0 and Insert
#define KC_KPDOT 0x63 // Keypad . and Delete

#define KC_102ND 0x64 // Keyboard Non-US \ and |
#define KC_COMPOSE 0x65 // Keyboard Application
#define KC_POWER 0x66 // Keyboard Power
#define KC_KPEQUAL 0x67 // Keypad =

#define KC_F13 0x68 // Keyboard F13
#define KC_F14 0x69 // Keyboard F14
#define KC_F15 0x6a // Keyboard F15
#define KC_F16 0x6b // Keyboard F16
#define KC_F17 0x6c // Keyboard F17
#define KC_F18 0x6d // Keyboard F18
#define KC_F19 0x6e // Keyboard F19
#define KC_F20 0x6f // Keyboard F20
#define KC_F21 0x70 // Keyboard F21
#define KC_F22 0x71 // Keyboard F22
#define KC_F23 0x72 // Keyboard F23
#define KC_F24 0x73 // Keyboard F24

#define KC_OPEN 0x74 // Keyboard Execute
#define KC_HELP 0x75 // Keyboard Help
#define KC_PROPS 0x76 // Keyboard Menu
#define KC_FRONT 0x77 // Keyboard Select
#define KC_STOP 0x78 // Keyboard Stop
#define KC_AGAIN 0x79 // Keyboard Again
#define KC_UNDO 0x7a // Keyboard Undo
#define KC_CUT 0x7b // Keyboard Cut
#define KC_COPY 0x7c // Keyboard Copy
#define KC_PASTE 0x7d // Keyboard Paste
#define KC_FIND 0x7e // Keyboard Find
#define KC_MUTE 0x7f // Keyboard Mute
#define KC_VOLUMEUP 0x80 // Keyboard Volume Up
#define KC_VOLUMEDOWN 0x81 // Keyboard Volume Down
// 0x82  Keyboard Locking Caps Lock
// 0x83  Keyboard Locking Num Lock
// 0x84  Keyboard Locking Scroll Lock
#define KC_KPCOMMA 0x85 // Keypad Comma
// 0x86  Keypad Equal Sign
#define KC_RO 0x87 // Keyboard International1
#define KC_KATAKANAHIRAGANA 0x88 // Keyboard International2
#define KC_YEN 0x89 // Keyboard International3
#define KC_HENKAN 0x8a // Keyboard International4
#define KC_MUHENKAN 0x8b // Keyboard International5
#define KC_KPJPCOMMA 0x8c // Keyboard International6
// 0x8d  Keyboard International7
// 0x8e  Keyboard International8
// 0x8f  Keyboard International9
#define KC_HANGEUL 0x90 // Keyboard LANG1
#define KC_HANJA 0x91 // Keyboard LANG2
#define KC_KATAKANA 0x92 // Keyboard LANG3
#define KC_HIRAGANA 0x93 // Keyboard LANG4
#define KC_ZENKAKUHANKAKU 0x94 // Keyboard LANG5
// 0x95  Keyboard LANG6
// 0x96  Keyboard LANG7
// 0x97  Keyboard LANG8
// 0x98  Keyboard LANG9
// 0x99  Keyboard Alternate Erase
// 0x9a  Keyboard SysReq/Attention
// 0x9b  Keyboard Cancel
// 0x9c  Keyboard Clear
// 0x9d  Keyboard Prior
// 0x9e  Keyboard Return
// 0x9f  Keyboard Separator
// 0xa0  Keyboard Out
// 0xa1  Keyboard Oper
// 0xa2  Keyboard Clear/Again
// 0xa3  Keyboard CrSel/Props
// 0xa4  Keyboard ExSel

// 0xb0  Keypad 00
// 0xb1  Keypad 000
// 0xb2  Thousands Separator
// 0xb3  Decimal Separator
// 0xb4  Currency Unit
// 0xb5  Currency Sub-unit
#define KC_KPLEFTPAREN 0xb6 // Keypad (
#define KC_KPRIGHTPAREN 0xb7 // Keypad )
// 0xb8  Keypad {
// 0xb9  Keypad }
// 0xba  Keypad Tab
// 0xbb  Keypad Backspace
// 0xbc  Keypad A
// 0xbd  Keypad B
// 0xbe  Keypad C
// 0xbf  Keypad D
// 0xc0  Keypad E
// 0xc1  Keypad F
// 0xc2  Keypad XOR
// 0xc3  Keypad ^
// 0xc4  Keypad %
// 0xc5  Keypad <
// 0xc6  Keypad >
// 0xc7  Keypad &
// 0xc8  Keypad &&
// 0xc9  Keypad |
// 0xca  Keypad ||
// 0xcb  Keypad :
// 0xcc  Keypad #
// 0xcd  Keypad Space
// 0xce  Keypad @
// 0xcf  Keypad !
// 0xd0  Keypad Memory Store
// 0xd1  Keypad Memory Recall
// 0xd2  Keypad Memory Clear
// 0xd3  Keypad Memory Add
// 0xd4  Keypad Memory Subtract
// 0xd5  Keypad Memory Multiply
// 0xd6  Keypad Memory Divide
// 0xd7  Keypad +/-
// 0xd8  Keypad Clear
// 0xd9  Keypad Clear Entry
// 0xda  Keypad Binary
// 0xdb  Keypad Octal
// 0xdc  Keypad Decimal
// 0xdd  Keypad Hexadecimal

#define KC_LEFTCTRL 0xe0 // Keyboard Left Control
#define KC_LEFTSHIFT 0xe1 // Keyboard Left Shift
#define KC_LEFTALT 0xe2 // Keyboard Left Alt
#define KC_LEFTMETA 0xe3 // Keyboard Left GUI
#define KC_RIGHTCTRL 0xe4 // Keyboard Right Control
#define KC_RIGHTSHIFT 0xe5 // Keyboard Right Shift
#define KC_RIGHTALT 0xe6 // Keyboard Right Alt
#define KC_RIGHTMETA 0xe7 // Keyboard Right GUI

#define KC_MEDIA_PLAYPAUSE 0xe8
#define KC_MEDIA_STOPCD 0xe9
#define KC_MEDIA_PREVIOUSSONG 0xea
#define KC_MEDIA_NEXTSONG 0xeb
#define KC_MEDIA_EJECTCD 0xec
#define KC_MEDIA_VOLUMEUP 0xed
#define KC_MEDIA_VOLUMEDOWN 0xee
#define KC_MEDIA_MUTE 0xef
#define KC_MEDIA_WWW 0xf0
#define KC_MEDIA_BACK 0xf1
#define KC_MEDIA_FORWARD 0xf2
#define KC_MEDIA_STOP 0xf3
#define KC_MEDIA_FIND 0xf4
#define KC_MEDIA_SCROLLUP 0xf5
#define KC_MEDIA_SCROLLDOWN 0xf6
#define KC_MEDIA_EDIT 0xf7
#define KC_MEDIA_SLEEP 0xf8
#define KC_MEDIA_COFFEE 0xf9
#define KC_MEDIA_REFRESH 0xfa
#define KC_MEDIA_CALC 0xfb
