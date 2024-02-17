/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024, All Rights Reserved.

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

#include "usbip_server_api.h"
#include "usb_mouse_descriptors.h"

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    (void)data;
    (void)data_length;
    uint8_t bRequestType = setup[0];
    uint8_t bRequest = setup[1];

    if ((bRequestType == 0x81) && (bRequest == USB_REQUEST_GET_DESCRIPTOR)) {
        if (setup[3] == 0x22) {
            uint16_t length = setup[7] << 8 | setup[6];
            usbip_device_transmit(0, usb_mouse_hid_report, length);
        }
    }
}

static void endpoint_data(uint8_t ep, const uint8_t *data, size_t data_length) {
    printf("EP (0x%x) Data: ", ep);
    for (size_t idx = 0; idx < data_length; idx++) {
        printf("0x%x (%c), ", data[idx], (char)data[idx]);
    }
    printf("\n");
}

static const usbip_device_t mouse_device = {
    .device_desc = usb_mouse_device,
    .config_desc = usb_mouse_configuration,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = endpoint_data,
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!usbip_server_start(&mouse_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB Mouse Device Activated\n");

    while (!usbip_server_interrupted()) {
        usleep(500000);
        uint8_t out_buffer[4];
        out_buffer[0] = 0;
        out_buffer[1] = 10;
        out_buffer[2] = 10;
        out_buffer[3] = 0;
        usbip_device_transmit(USB_MOUSE_HID_EP_DATA_IN, out_buffer, sizeof(out_buffer));
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}
