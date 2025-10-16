/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2025, All Rights Reserved.

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
#include "usb_bulk_descriptors.h"

#define INPUT_BUFFER_LENGTH 512

static uint8_t input_buffer[INPUT_BUFFER_LENGTH];
static volatile size_t input_buffer_pos = 0;

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    printf("Control Data (%zu)\n", data_length);
    (void)setup;
    (void)data;
}

static void endpoint_data(uint8_t ep, const uint8_t *data, size_t data_length) {
    (void)ep;

    if ((input_buffer_pos + data_length) <= sizeof(input_buffer)) {
        memcpy(&input_buffer[input_buffer_pos], data, data_length);
        input_buffer_pos += data_length;
    }
}

static const usbip_device_t bulk_device = {
    .device_desc = usb_device_descriptor,
    .config_desc = bulk_config_descriptor,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = endpoint_data,
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!usbip_server_start(&bulk_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB BULK Device Activated\n");

    while (!usbip_server_interrupted()) {
        static int msg_counter = 0;
        bool reply = false;

        /**
         * Enter critical section before messing with variables updated
         * in interrupt context (input_buffer and input_buffer_pos
         */
        usbip_device_critical_section_enter();
        if (input_buffer_pos > 0) {
            printf("Input: ");
            for (size_t idx = 0; idx < input_buffer_pos; idx++) {
                printf("%c", input_buffer[idx]);
            }
            printf("\n");
            input_buffer_pos = 0;

            msg_counter++;
            reply = true;
        }
        usbip_device_critical_section_exit();

        if (reply) {
            char str_buffer[512];
            snprintf(str_buffer, sizeof(str_buffer), "Message Counter: %d", msg_counter);
            usbip_device_transmit(1, (uint8_t *)str_buffer, strlen(str_buffer));
        }

        usleep(500000);
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}
