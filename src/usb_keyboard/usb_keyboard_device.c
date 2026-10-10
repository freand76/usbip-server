/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2025, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "usbip_server_api.h"
#include "usb_keyboard_descriptors.h"

typedef struct {
        uint8_t keycode;  // USB HID keycode
        uint8_t modifier; // Modifier mask (e.g., 0x02 = Left Shift)
} hid_key_t;

// Modifier bits (HID usage 0xE0–0xE7)
#define MOD_LCTRL 0x01
#define MOD_LSHIFT 0x02
#define MOD_LALT 0x04
#define MOD_LGUI 0x08
#define MOD_RCTRL 0x10
#define MOD_RSHIFT 0x20
#define MOD_RALT 0x40
#define MOD_RGUI 0x80

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length);
static hid_key_t ascii_to_hid(char c);

static const usbip_device_t keyboard_device = {
    .device_desc = usb_keyboard_device,
    .config_desc = usb_keyboard_configuration,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = NULL,
};

volatile bool configuration_active = false;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    const char typing_string[] = "Who is typing...";

    if (!usbip_server_start(&keyboard_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB Keyboard Device Activated\n");

    while (!usbip_server_interrupted()) {
        while (!usbip_server_interrupted()) {
            usleep(100000);
            usbip_device_critical_section_enter();
            bool is_configuration_active = configuration_active;
            configuration_active = false;
            usbip_device_critical_section_exit();

            if (is_configuration_active) {
                printf("Configuration Active\n");
                break;
            }
        }

        for (size_t string_idx = 0; string_idx < strlen(typing_string); string_idx++) {
            if (usbip_server_interrupted()) {
                break;
            }
            usleep(100000);

            // [0] Modifier
            // [1] Reserved
            // [2] Keycode 0
            // [3] Keycode 1
            // [4] Keycode 2
            // [5] Keycode 3
            // [6] Keycode 4
            // [7] Keycode 5
            uint8_t out_buffer[8] = {0};

            // Get character
            char char_to_transmit = typing_string[string_idx];
            hid_key_t hid_key = ascii_to_hid(char_to_transmit);

            // Send keydown report
            out_buffer[0] = hid_key.modifier;
            out_buffer[2] = hid_key.keycode;
            usbip_device_transmit(USB_KEYBOARD_HID_EP_DATA_IN, out_buffer, sizeof(out_buffer));

            // Wait for the keystroke to be recognized (20ms)
            usleep(20000);

            // Send all zero report
            memset(out_buffer, 0, sizeof(out_buffer));
            usbip_device_transmit(USB_KEYBOARD_HID_EP_DATA_IN, out_buffer, sizeof(out_buffer));
        }
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}

static hid_key_t ascii_to_hid(char c) {
    hid_key_t result = {0, 0};

    if (c >= 'a' && c <= 'z') {
        result.keycode = (c - 'a') + 0x04; // 'a' → 0x04
    } else if (c >= 'A' && c <= 'Z') {
        result.keycode = (c - 'A') + 0x04;
        result.modifier = MOD_LSHIFT;
    } else if (c >= '1' && c <= '9') {
        result.keycode = (c - '1') + 0x1E; // '1' → 0x1E
    } else if (c == '0') {
        result.keycode = 0x27;
    } else {
        switch (c) {
        case '\n':
            result.keycode = 0x28;
            break; // Enter
        case '\b':
            result.keycode = 0x2A;
            break; // Backspace
        case '\t':
            result.keycode = 0x2B;
            break; // Tab
        case ' ':
            result.keycode = 0x2C;
            break; // Space

        // Symbols
        case '-':
            result.keycode = 0x2D;
            break;
        case '=':
            result.keycode = 0x2E;
            break;
        case '[':
            result.keycode = 0x2F;
            break;
        case ']':
            result.keycode = 0x30;
            break;
        case '\\':
            result.keycode = 0x31;
            break;
        case ';':
            result.keycode = 0x33;
            break;
        case '\'':
            result.keycode = 0x34;
            break;
        case '`':
            result.keycode = 0x35;
            break;
        case ',':
            result.keycode = 0x36;
            break;
        case '.':
            result.keycode = 0x37;
            break;
        case '/':
            result.keycode = 0x38;
            break;

        // Shifted symbols
        case '!':
            result.keycode = 0x1E;
            result.modifier = MOD_LSHIFT;
            break;
        case '@':
            result.keycode = 0x1F;
            result.modifier = MOD_LSHIFT;
            break;
        case '#':
            result.keycode = 0x20;
            result.modifier = MOD_LSHIFT;
            break;
        case '$':
            result.keycode = 0x21;
            result.modifier = MOD_LSHIFT;
            break;
        case '%':
            result.keycode = 0x22;
            result.modifier = MOD_LSHIFT;
            break;
        case '^':
            result.keycode = 0x23;
            result.modifier = MOD_LSHIFT;
            break;
        case '&':
            result.keycode = 0x24;
            result.modifier = MOD_LSHIFT;
            break;
        case '*':
            result.keycode = 0x25;
            result.modifier = MOD_LSHIFT;
            break;
        case '(':
            result.keycode = 0x26;
            result.modifier = MOD_LSHIFT;
            break;
        case ')':
            result.keycode = 0x27;
            result.modifier = MOD_LSHIFT;
            break;
        case '_':
            result.keycode = 0x2D;
            result.modifier = MOD_LSHIFT;
            break;
        case '+':
            result.keycode = 0x2E;
            result.modifier = MOD_LSHIFT;
            break;
        case '{':
            result.keycode = 0x2F;
            result.modifier = MOD_LSHIFT;
            break;
        case '}':
            result.keycode = 0x30;
            result.modifier = MOD_LSHIFT;
            break;
        case '|':
            result.keycode = 0x31;
            result.modifier = MOD_LSHIFT;
            break;
        case ':':
            result.keycode = 0x33;
            result.modifier = MOD_LSHIFT;
            break;
        case '"':
            result.keycode = 0x34;
            result.modifier = MOD_LSHIFT;
            break;
        case '~':
            result.keycode = 0x35;
            result.modifier = MOD_LSHIFT;
            break;
        case '<':
            result.keycode = 0x36;
            result.modifier = MOD_LSHIFT;
            break;
        case '>':
            result.keycode = 0x37;
            result.modifier = MOD_LSHIFT;
            break;
        case '?':
            result.keycode = 0x38;
            result.modifier = MOD_LSHIFT;
            break;
        default:
            result.keycode = 0; // Unknown
            break;
        }
    }

    return result;
}

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    (void)data;
    (void)data_length;
    uint8_t bRequestType = setup[0];
    uint8_t bRequest = setup[1];

    if ((bRequestType == 0x81) && (bRequest == USB_REQUEST_GET_DESCRIPTOR)) {
        if (setup[3] == USB_DESCRIPTOR_TYPE_REPORT) {
            uint16_t length = setup[7] << 8 | setup[6];
            if (length > sizeof(usb_keyboard_hid_report)) {
                length = sizeof(usb_keyboard_hid_report);
            }
            usbip_device_transmit(0, usb_keyboard_hid_report, length);
        }
    } else if ((bRequestType == 0x21) && (bRequest == USB_REQUEST_SET_CONFIGURATION)) {
        // Set Configuration
        configuration_active = true;
    }
}
