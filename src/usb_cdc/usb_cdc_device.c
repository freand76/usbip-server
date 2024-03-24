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
#include "usb_cdc_descriptors.h"

#define CDC_SET_LINE_CODING (0x20)
#define CDC_SET_CONTROL_LINE_STATE (0x22)
#define CDC_SEND_BREAK (0x23)

#define INPUT_BUFFER_LENGTH 10

typedef struct {
        bool DTR;
        bool RTS;
        uint32_t baudrate;
        uint8_t char_format;
        uint8_t parity;
        uint8_t data_bits;
} cdc_state_t;

static volatile cdc_state_t cdc_state = {0};

static uint8_t input_buffer[INPUT_BUFFER_LENGTH];
static volatile size_t input_buffer_pos = 0;

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    uint8_t bRequest = setup[1];
    uint16_t wValue = (setup[3] << 8) | setup[2];

    switch (bRequest) {
    case CDC_SET_LINE_CODING:
        if (data_length == 7) {
            cdc_state.baudrate = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
            cdc_state.char_format = data[4];
            cdc_state.parity = data[5];
            cdc_state.data_bits = data[6];
            printf("Linecoding: Baud=%d CharFormat=%d Parity=%d DataBits=%d\n", cdc_state.baudrate,
                   cdc_state.char_format, cdc_state.parity, cdc_state.data_bits);
        }
        break;
    case CDC_SET_CONTROL_LINE_STATE:
        cdc_state.DTR = (wValue & 0x01) != 0;
        cdc_state.RTS = (wValue & 0x02) != 0;
        printf("Linestate: DTR: %s RTS: %s\n", cdc_state.DTR ? "HIGH" : "LOW ",
               cdc_state.RTS ? "HIGH" : "LOW ");
        break;
    case CDC_SEND_BREAK:
        printf("Linebreak: 0x%" PRIx16 "\n", wValue);
        break;
    default:
        break;
    }
}

static void endpoint_data(uint8_t ep, const uint8_t *data, size_t data_length) {
    (void)ep;

    if ((input_buffer_pos + data_length) <= sizeof(input_buffer)) {
        memcpy(&input_buffer[input_buffer_pos], data, data_length);
        input_buffer_pos += data_length;
    }
}

static const usbip_device_t cdc_device = {
    .device_desc = usb_device_descriptor,
    .config_desc = cdc_config_descriptor,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = endpoint_data,
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!usbip_server_start(&cdc_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB CDC Device Activated\n");

    while (!usbip_server_interrupted()) {
        static uint32_t tick_counter = 0;
        static char str_buffer[100];

        /* DTR goes high when a terminal connects */
        if (cdc_state.DTR) {
            snprintf(str_buffer, sizeof(str_buffer), "Tick %d\n", tick_counter++);
            usbip_device_transmit(1, (uint8_t *)str_buffer, strlen(str_buffer));
        }

        /**
         * Enter critical section before messing with variables updated
         * in interrupt context (input_buffer and input_buffer_pos
         */
        usbip_device_critical_section_enter();
        if (input_buffer_pos > 0) {
            printf("Input: ");
            for (size_t idx = 0; idx < input_buffer_pos; idx++) {
                printf("0x%.2x, ", input_buffer[idx]);
            }
            printf("\n");
            input_buffer_pos = 0;
        }
        usbip_device_critical_section_exit();

        usleep(500000);
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}
