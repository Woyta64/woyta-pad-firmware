#include "tusb.h"
#include "generated_config.h"

// --- ENUMERATE INTERFACES ---
enum {
    ITF_NUM_KEYBOARD = 0,
    ITF_NUM_CUSTOM, // The WebHID interface
    ITF_NUM_TOTAL
};

// --- ENUMERATE ENDPOINTS ---
#define EPNUM_KEYBOARD   0x81 // IN pipe (Keyboard -> PC)
#define EPNUM_CUSTOM     0x82 // IN pipe (Keyboard -> PC for WebHID)
#define EPNUM_CUSTOM_OUT 0x02 // OUT pipe (PC -> Keyboard for WebHID)

// --- 1. DEVICE DESCRIPTOR ---
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_BCD,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

// --- 2. HID REPORT DESCRIPTORS ---

// Interface 0: Standard Keyboard Report
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// Interface 1: Custom WebHID Report (Vendor Defined 0xFF60)
// This establishes a 32-byte two-way communication channel
uint8_t const desc_hid_custom_report[] = {
    HID_USAGE_PAGE_N ( 0xFF60, 2 ),
    HID_USAGE        ( 0x61 ),
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),
        // Input report (Keyboard to PC)
        HID_LOGICAL_MIN  ( 0x00 ),
        HID_LOGICAL_MAX_N( 0x00FF, 2 ),
        HID_REPORT_SIZE  ( 8 ),
        HID_REPORT_COUNT ( 32 ),
        HID_USAGE        ( 0x62 ),
        HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        // Output report (PC to Keyboard)
        HID_USAGE        ( 0x63 ),
        HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NON_VOLATILE ),
    HID_COLLECTION_END
};

// Route the correct descriptor based on the interface requested
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    if (instance == ITF_NUM_KEYBOARD) return desc_hid_report;
    if (instance == ITF_NUM_CUSTOM)   return desc_hid_custom_report;
    return NULL;
}

// --- 3. CONFIGURATION DESCRIPTOR ---

// The total length now includes the Config Header, the standard HID desc, and the IN/OUT HID desc
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config Header
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface 0: Keyboard (wMaxPacketSize=8 matches the 8-byte HID keyboard report)
    TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), EPNUM_KEYBOARD, 8, 10),

    // Interface 1: Custom WebHID
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_CUSTOM, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_custom_report), EPNUM_CUSTOM, EPNUM_CUSTOM_OUT, 32, 10)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

// --- 4. STRINGS ---
char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: English
    USB_MANUFACTURER,              // 1: Manufacturer
    USB_PRODUCT,                   // 2: Product
    "123456",                      // 3: Serial
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2 * chr_count + 2);
    return _desc_str;
}