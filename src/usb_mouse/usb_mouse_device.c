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
#include <math.h>

#include "usbip_server_api.h"
#include "usb_mouse_descriptors.h"

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    (void)data;
    (void)data_length;
    uint8_t bRequestType = setup[0];
    uint8_t bRequest = setup[1];

    if ((bRequestType == 0x81) && (bRequest == USB_REQUEST_GET_DESCRIPTOR)) {
        if (setup[3] == USB_DESCRIPTOR_TYPE_REPORT) {
            uint16_t length = setup[7] << 8 | setup[6];
            usbip_device_transmit(0, usb_mouse_hid_report, length);
        }
    }
}

static const usbip_device_t mouse_device = {
    .device_desc = usb_mouse_device,
    .config_desc = usb_mouse_configuration,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = NULL,
};

#define CIRCLE_RADIUS 200
#define CIRCLE_ROUND_TRIP_TIME_MS 1000
#define NUMBER_OF_STEPS (CIRCLE_ROUND_TRIP_TIME_MS / USB_MOUSE_HID_POLL_INTERVAL_MS)

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    uint16_t step_counter = 0;

    if (!usbip_server_start(&mouse_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB Mouse Device Activated\n");

    while (!usbip_server_interrupted()) {
        float old_angle = (2 * M_PI * step_counter) / NUMBER_OF_STEPS;
        int16_t old_pos_x = CIRCLE_RADIUS * cos(old_angle);
        int16_t old_pos_y = CIRCLE_RADIUS * sin(old_angle);
        step_counter++;
        if (step_counter >= NUMBER_OF_STEPS) {
            step_counter = 0;
        }
        float new_angle = (2 * M_PI * step_counter) / NUMBER_OF_STEPS;
        int16_t new_pos_x = CIRCLE_RADIUS * cos(new_angle);
        int16_t new_pos_y = CIRCLE_RADIUS * sin(new_angle);

        int16_t delta_x = new_pos_x - old_pos_x;
        int16_t delta_y = new_pos_y - old_pos_y;

        uint8_t out_buffer[4];
        out_buffer[0] = 0;
        out_buffer[1] = delta_x;
        out_buffer[2] = delta_y;
        out_buffer[3] = 0;
        usbip_device_transmit(USB_MOUSE_HID_EP_DATA_IN, out_buffer, sizeof(out_buffer));
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}
