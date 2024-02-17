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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "descriptor_utils.h"
#include "usbip_device.h"

pthread_mutex_t ep_access_mutex;

#define NUMBER_OF_ENDPOINTS (16)
#define ENDPOINT_MAX_DATA_LENGTH (512)

#define USB_REQUEST_RECIPIENT_DEVICE (0)
#define USB_REQUEST_RECIPIENT_INTERFACE (1)

typedef struct {
        uint8_t data[ENDPOINT_MAX_DATA_LENGTH];
        size_t data_length;
        pthread_mutex_t ep_lock;
} device_endpoint_data_t;

static bool device_stopped = false;
static device_endpoint_data_t endpoint_data[NUMBER_OF_ENDPOINTS];

static const char *get_string(const usbip_device_t *usb_device, uint8_t index);
static void device_request(const usbip_device_t *usb_device, uint8_t *usb_setup, uint8_t *buffer,
                           size_t req_size);
static void interface_request(const usbip_device_t *usb_device, uint8_t *usb_setup, uint8_t *buffer,
                              size_t req_size);

void usbip_device_stop(void) {
    device_stopped = true;
    for (size_t idx = 0; idx < NUMBER_OF_ENDPOINTS; idx++) {
        usbip_device_release_buffer(idx);
    }
}

void usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length) {
    uint8_t ep_masked = ep & 0xf;
    if (!device_stopped) {
        pthread_mutex_lock(&endpoint_data[ep_masked].ep_lock);
        pthread_mutex_lock(&ep_access_mutex);
        memcpy(endpoint_data[ep_masked].data, data, data_length);
        endpoint_data[ep_masked].data_length = data_length;
        pthread_mutex_unlock(&ep_access_mutex);
    }
}

uint8_t *usbip_device_get_ep_buffer(uint8_t ep, size_t *buffer_size) {
    uint8_t ep_masked = ep & 0xf;
    uint8_t *buffer = NULL;
    pthread_mutex_lock(&ep_access_mutex);
    buffer = endpoint_data[ep_masked].data;
    *buffer_size = endpoint_data[ep_masked].data_length;
    pthread_mutex_unlock(&ep_access_mutex);
    return buffer;
}

void usbip_device_release_buffer(uint8_t ep) {
    uint8_t ep_masked = ep & 0xf;
    pthread_mutex_lock(&ep_access_mutex);
    memset(endpoint_data[ep_masked].data, 0, ENDPOINT_MAX_DATA_LENGTH);
    endpoint_data[ep_masked].data_length = 0;
    pthread_mutex_unlock(&ep_access_mutex);
    pthread_mutex_unlock(&endpoint_data[ep_masked].ep_lock);
}

void usbip_device_control(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                          size_t length) {
    uint8_t bmRequestType = setup[0];
    uint8_t bfRecipient = bmRequestType & 0x05;

    if (bmRequestType & 0x20) {
        /* Vendor */
        usb_device->control_data_callback(setup, buffer, length);
    } else if (bfRecipient == USB_REQUEST_RECIPIENT_DEVICE) {
        device_request(usb_device, setup, buffer, length);
    } else if (bfRecipient == USB_REQUEST_RECIPIENT_INTERFACE) {
        interface_request(usb_device, setup, buffer, length);
    } else {
        printf("USBIPDevice: Unknown bfRecipient = %x\n", bfRecipient);
    }
}

//
// USB CONTROL STACK
//

static const char *get_string(const usbip_device_t *usb_device, uint8_t index) {
    const char *string = NULL;
    if (usb_device != NULL) {
        const usb_string_t *usb_string = usb_device->usb_strings;
        while (true) {
            if (usb_string->id == index) {
                string = usb_string->string;
                break;
            } else if (usb_string->id == 0) {
                break;
            }
            usb_string++;
        }
    }
    return string;
}

static void device_request(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                           size_t req_size) {
    (void)buffer;

    uint8_t bmRequestType = setup[0];
    uint8_t bRequest = setup[1];
    uint8_t bfDataDirection = bmRequestType & 0x80;
    if (bfDataDirection != 0) {
        if (bRequest == USB_REQUEST_GET_STATUS) {
            uint8_t tx_buffer[2];
            tx_buffer[0] = 0x00;
            tx_buffer[1] = 0x01;
            usbip_device_transmit(0, tx_buffer, sizeof(tx_buffer));
        } else if (bRequest == USB_REQUEST_GET_DESCRIPTOR) {

            uint8_t bDescriptorType = setup[3];
            uint8_t bDescriptorIndex = setup[2];
            if (bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE) {
                usbip_device_transmit(0, usb_device->device_desc, req_size);
            } else if (bDescriptorType == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
                usbip_device_transmit(0, usb_device->config_desc, req_size);
            } else if (bDescriptorType == USB_DESCRIPTOR_TYPE_STRING) {
                if (bDescriptorIndex == 0) {
                    size_t tx_length = (req_size < usb_device->langid_desc[0])
                                           ? req_size
                                           : usb_device->langid_desc[0];
                    usbip_device_transmit(0, usb_device->langid_desc, tx_length);
                } else {
                    const char *string = get_string(usb_device, bDescriptorIndex);
                    if (string != NULL) {
                        uint8_t string_len = strlen(string);
                        uint8_t string_buffer[512];
                        string_buffer[0] = 2 * string_len + 2;
                        string_buffer[1] = 3;
                        for (size_t idx = 0; idx < strlen(string); idx++) {
                            string_buffer[2 + 2 * idx + 0] = (uint8_t)string[idx];
                            string_buffer[2 + 2 * idx + 1] = 0;
                        }
                        size_t tx_length =
                            (req_size < string_buffer[0]) ? req_size : string_buffer[0];
                        usbip_device_transmit(0, string_buffer, tx_length);
                    }
                }
            }
        } else {
            printf("USBIPDevice: Unknown bRequest: %.2x", bRequest);
        }
    }
}

static void interface_request(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                              size_t req_size) {
    /* Let device handle interface requests */
    usb_device->control_data_callback(setup, buffer, req_size);
}
