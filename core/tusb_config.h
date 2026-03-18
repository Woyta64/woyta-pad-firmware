#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // Use the native USB port on RP2040
#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE
#endif

    // Enable Device Stack
#define CFG_TUD_ENABLED       1

    // 2 HID Interfaces: Interface 0 = Keyboard, Interface 1 = WebHID configurator
#define CFG_TUD_HID           2
#define CFG_TUD_CDC           0
#define CFG_TUD_MSC           0
#define CFG_TUD_MIDI          0
#define CFG_TUD_VENDOR        0

    // Must match WebHID report size (32 bytes). TinyUSB arms the OUT endpoint with
    // CFG_TUD_HID_EP_BUFSIZE as the transfer length. If this is larger than wMaxPacketSize (32),
    // the RP2040 USB controller waits for more data before completing the transfer, causing
    // two sendReport() calls to be concatenated into one callback invocation.
#define CFG_TUD_HID_EP_BUFSIZE    32

#ifdef __cplusplus
}
#endif