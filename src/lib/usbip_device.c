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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "descriptor_utils.h"
#include "usbip_device.h"

#define NUMBER_OF_ENDPOINTS (16)
#define ENDPOINT_MAX_DATA_LENGTH (512)

#define USB_REQUEST_TYPE_STANDARD (0)

#define USB_TRANSFER_TYPE_BULK (2)
#define USB_TRANSFER_TYPE_INTERRUPT (3)
#define FULL_SPEED_MAX_PACKET_SIZE (64)

#define USB_REQUEST_RECIPIENT_DEVICE (0)
#define USB_REQUEST_RECIPIENT_INTERFACE (1)
#define USB_REQUEST_RECIPIENT_ENDPOINT (2)

typedef struct {
        uint8_t data[ENDPOINT_MAX_DATA_LENGTH];
        size_t data_length;
} device_endpoint_data_t;

static pthread_mutex_t receive_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ep_access_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ep_data_consumed = PTHREAD_COND_INITIALIZER;
static bool device_stopped = false;
static device_endpoint_data_t endpoint_data[NUMBER_OF_ENDPOINTS];
static int ep_data_event_fd = -1;
static uint8_t configuration_value = 0;

/* The server thread fetches endpoint data, so neither it (where all device callbacks run) nor a
   thread blocking it with a critical section may wait for the data to be fetched */
static __thread bool is_server_thread = false;
static __thread bool in_critical_section = false;

static const char *get_string(const usbip_device_t *usb_device, uint8_t index);
static bool device_request(const usbip_device_t *usb_device, uint8_t *usb_setup, uint8_t *buffer,
                           size_t req_size);
static bool endpoint_request(uint8_t *usb_setup);
static size_t build_other_speed_config(const uint8_t *config_desc, uint8_t *buffer,
                                       size_t buffer_length);
static void interface_request(const usbip_device_t *usb_device, uint8_t *usb_setup, uint8_t *buffer,
                              size_t req_size);

bool usbip_device_init(void) {
    ep_data_event_fd = eventfd(0, EFD_NONBLOCK);
    return (ep_data_event_fd >= 0);
}

void usbip_device_stop(void) {
    pthread_mutex_lock(&ep_access_mutex);
    device_stopped = true;
    for (size_t idx = 0; idx < NUMBER_OF_ENDPOINTS; idx++) {
        endpoint_data[idx].data_length = 0;
    }
    pthread_cond_broadcast(&ep_data_consumed);
    pthread_mutex_unlock(&ep_access_mutex);
}

void usbip_device_receive(const usbip_device_t *usb_device, uint8_t ep, uint8_t *data,
                          size_t data_length) {
    pthread_mutex_lock(&receive_lock);
    usb_device->ep_data_callback(ep, data, data_length);
    pthread_mutex_unlock(&receive_lock);
}

void usbip_device_mark_server_thread(void) {
    is_server_thread = true;
}

bool usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length) {
    uint8_t ep_masked = ep & 0xf;
    if (data_length > ENDPOINT_MAX_DATA_LENGTH) {
        data_length = ENDPOINT_MAX_DATA_LENGTH;
    }

    /* Control replies are complete when the request returns and may replace older data */
    bool replace = (ep_masked == 0);
    bool may_wait = !is_server_thread && !in_critical_section;
    bool stored = false;

    pthread_mutex_lock(&ep_access_mutex);
    /* Wait for the host to fetch earlier data */
    while (!replace && may_wait && !device_stopped && (endpoint_data[ep_masked].data_length > 0)) {
        pthread_cond_wait(&ep_data_consumed, &ep_access_mutex);
    }
    bool stopped = device_stopped;
    if (!stopped && (replace || (endpoint_data[ep_masked].data_length == 0))) {
        memcpy(endpoint_data[ep_masked].data, data, data_length);
        endpoint_data[ep_masked].data_length = data_length;
        stored = true;
    }
    pthread_mutex_unlock(&ep_access_mutex);

    if (!stored && !stopped) {
        fprintf(stderr,
                "USBIPDevice: EP%u still has unread data, dropped %zu bytes "
                "(transmit can not wait in a callback or critical section)\n",
                ep_masked, data_length);
    }

    if (stored && (ep_masked != 0)) {
        /* Wake the server thread so pending IN transfers can complete */
        uint64_t event = 1;
        if (write(ep_data_event_fd, &event, sizeof(event)) < 0) {
            /* Counter is already non-zero, the server will wake anyway */
        }
    }
    return stored;
}

void usbip_device_critical_section_enter(void) {
    pthread_mutex_lock(&receive_lock);
    in_critical_section = true;
}

void usbip_device_critical_section_exit(void) {
    in_critical_section = false;
    pthread_mutex_unlock(&receive_lock);
}

int usbip_device_get_event_fd(void) {
    return ep_data_event_fd;
}

void usbip_device_clear_event(void) {
    uint64_t event;
    if (read(ep_data_event_fd, &event, sizeof(event)) < 0) {
        /* No event pending */
    }
}

size_t usbip_device_fetch_ep_data(uint8_t ep, uint8_t *buffer, size_t max_length) {
    uint8_t ep_masked = ep & 0xf;
    pthread_mutex_lock(&ep_access_mutex);
    size_t available = endpoint_data[ep_masked].data_length;
    size_t length = (max_length < available) ? max_length : available;
    size_t remaining = available - length;
    memcpy(buffer, endpoint_data[ep_masked].data, length);
    if (ep_masked == 0) {
        /* Control transfer is complete, drop anything beyond wLength */
        remaining = 0;
    }
    memmove(endpoint_data[ep_masked].data, &endpoint_data[ep_masked].data[length], remaining);
    endpoint_data[ep_masked].data_length = remaining;
    if (remaining == 0) {
        pthread_cond_broadcast(&ep_data_consumed);
    }
    pthread_mutex_unlock(&ep_access_mutex);
    return length;
}

bool usbip_device_control(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                          size_t length) {
    uint8_t bmRequestType = setup[0];
    uint8_t bfType = (bmRequestType >> 5) & 0x03;
    uint8_t bfRecipient = bmRequestType & 0x1f;

    bool supported = true;
    if (bfType != USB_REQUEST_TYPE_STANDARD) {
        /* Class or Vendor */
        usb_device->control_data_callback(setup, buffer, length);
    } else if (bfRecipient == USB_REQUEST_RECIPIENT_DEVICE) {
        supported = device_request(usb_device, setup, buffer, length);
    } else if (bfRecipient == USB_REQUEST_RECIPIENT_INTERFACE) {
        interface_request(usb_device, setup, buffer, length);
    } else if (bfRecipient == USB_REQUEST_RECIPIENT_ENDPOINT) {
        supported = endpoint_request(setup);
    } else {
        printf("USBIPDevice: Unknown bfRecipient = %x\n", bfRecipient);
        supported = false;
    }
    return supported;
}

//
// USB CONTROL STACK
//

static const char *get_string(const usbip_device_t *usb_device, uint8_t index) {
    const char *string = NULL;
    if (usb_device != NULL) {
        const usb_string_t *usb_string = usb_device->usb_strings;
        while (usb_string != NULL) {
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

static bool device_request(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                           size_t req_size) {
    (void)buffer;
    bool supported = true;

    uint8_t bmRequestType = setup[0];
    uint8_t bRequest = setup[1];
    uint8_t bfDataDirection = bmRequestType & 0x80;
    if (bfDataDirection != 0) {
        if (bRequest == USB_REQUEST_GET_STATUS) {
            /* Bit 0 is self powered, remote wakeup (bit 1) can never be enabled */
            uint8_t self_powered = (usb_device->config_desc[7] & 0x40) ? 0x01 : 0x00;
            uint8_t tx_buffer[2] = {self_powered, 0x00};
            usbip_device_transmit(0, tx_buffer, sizeof(tx_buffer));
        } else if (bRequest == USB_REQUEST_GET_DESCRIPTOR) {

            uint8_t bDescriptorType = setup[3];
            uint8_t bDescriptorIndex = setup[2];
            if (bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE) {
                size_t desc_length = usb_device->device_desc[0];
                size_t tx_length = (req_size < desc_length) ? req_size : desc_length;
                usbip_device_transmit(0, usb_device->device_desc, tx_length);
            } else if (bDescriptorType == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
                size_t desc_length = (usb_device->config_desc[3] << 8) | usb_device->config_desc[2];
                size_t tx_length = (req_size < desc_length) ? req_size : desc_length;
                usbip_device_transmit(0, usb_device->config_desc, tx_length);
            } else if ((bDescriptorType == USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER) &&
                       (usb_device->device_desc[3] >= 0x02)) {
                /* Only USB 2.0 devices have a qualifier, the other speed uses the same
                   device descriptor values */
                const uint8_t *device_desc = usb_device->device_desc;
                uint8_t qualifier_desc[10] = {
                    sizeof(qualifier_desc),
                    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
                    device_desc[2], // bcdUSB
                    device_desc[3],
                    device_desc[4],  // bDeviceClass
                    device_desc[5],  // bDeviceSubClass
                    device_desc[6],  // bDeviceProtocol
                    device_desc[7],  // bMaxPacketSize0
                    device_desc[17], // bNumConfigurations
                    0,               // bReserved
                };
                size_t tx_length =
                    (req_size < sizeof(qualifier_desc)) ? req_size : sizeof(qualifier_desc);
                usbip_device_transmit(0, qualifier_desc, tx_length);
            } else if ((bDescriptorType == USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION) &&
                       (usb_device->device_desc[3] >= 0x02)) {
                uint8_t other_speed_desc[ENDPOINT_MAX_DATA_LENGTH];
                size_t desc_length = build_other_speed_config(
                    usb_device->config_desc, other_speed_desc, sizeof(other_speed_desc));
                size_t tx_length = (req_size < desc_length) ? req_size : desc_length;
                usbip_device_transmit(0, other_speed_desc, tx_length);
            } else if (bDescriptorType == USB_DESCRIPTOR_TYPE_STRING) {
                if (bDescriptorIndex == 0) {
                    size_t tx_length = (req_size < usb_device->langid_desc[0])
                                           ? req_size
                                           : usb_device->langid_desc[0];
                    usbip_device_transmit(0, usb_device->langid_desc, tx_length);
                } else {
                    const char *string = get_string(usb_device, bDescriptorIndex);
                    if (string != NULL) {
                        size_t string_len = strlen(string);
                        // Limit string_len to prevent string_buffer overflow (2 * string_len + 2 <=
                        // 255)
                        if (string_len > 126) {
                            string_len = 126;
                        }

                        uint8_t string_buffer[512];
                        string_buffer[0] = 2 * string_len + 2;
                        string_buffer[1] = 3;
                        for (size_t idx = 0; idx < string_len; idx++) {
                            string_buffer[2 + 2 * idx + 0] = (uint8_t)string[idx];
                            string_buffer[2 + 2 * idx + 1] = 0;
                        }
                        size_t tx_length =
                            (req_size < string_buffer[0]) ? req_size : string_buffer[0];
                        usbip_device_transmit(0, string_buffer, tx_length);
                    }
                }
            }
        } else if (bRequest == USB_REQUEST_GET_CONFIGURATION) {
            usbip_device_transmit(0, &configuration_value, sizeof(configuration_value));
        } else {
            printf("USBIPDevice: Unknown bRequest: %.2x\n", bRequest);
        }
    } else if (bRequest == USB_REQUEST_SET_CONFIGURATION) {
        configuration_value = setup[2];
    } else {
        printf("USBIPDevice: Unsupported bRequest: %.2x\n", bRequest);
        supported = false;
    }
    return supported;
}

/* The configuration as seen by a full speed host, the device runs at high speed */
static size_t build_other_speed_config(const uint8_t *config_desc, uint8_t *buffer,
                                       size_t buffer_length) {
    size_t total_length = (config_desc[3] << 8) | config_desc[2];
    if (total_length > buffer_length) {
        total_length = buffer_length;
    }
    memcpy(buffer, config_desc, total_length);
    buffer[1] = USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION;

    size_t idx = 0;
    while ((idx + 7 <= total_length) && (buffer[idx] > 0)) {
        uint8_t *desc = &buffer[idx];
        uint8_t transfer_type = desc[3] & 0x03;
        if ((desc[1] == USB_DESCRIPTOR_TYPE_ENDPOINT) &&
            ((transfer_type == USB_TRANSFER_TYPE_BULK) ||
             (transfer_type == USB_TRANSFER_TYPE_INTERRUPT))) {
            /* Full speed bulk and interrupt packets are at most 64 bytes */
            if (((desc[5] << 8) | desc[4]) > FULL_SPEED_MAX_PACKET_SIZE) {
                desc[4] = FULL_SPEED_MAX_PACKET_SIZE;
                desc[5] = 0;
            }
            /* High speed interval is 2^(bInterval - 1) * 125 us, full speed is in ms */
            if ((transfer_type == USB_TRANSFER_TYPE_INTERRUPT) && (desc[6] > 0)) {
                uint8_t exponent = (desc[6] > 16) ? 16 : desc[6];
                size_t interval_ms = ((size_t)1 << (exponent - 1)) / 8;
                desc[6] = (interval_ms < 1) ? 1 : (interval_ms > 255) ? 255 : interval_ms;
            }
        }
        idx += desc[0];
    }
    return total_length;
}

static bool endpoint_request(uint8_t *setup) {
    uint8_t bRequest = setup[1];
    bool supported = true;
    if (bRequest == USB_REQUEST_GET_STATUS) {
        /* Endpoints are never halted */
        uint8_t tx_buffer[2] = {0x00, 0x00};
        usbip_device_transmit(0, tx_buffer, sizeof(tx_buffer));
    } else if ((bRequest == USB_REQUEST_CLEAR_FEATURE) || (bRequest == USB_REQUEST_SET_FEATURE)) {
        /* Accept halt requests, there is no halt state to change */
    } else {
        printf("USBIPDevice: Unsupported endpoint bRequest: %.2x\n", bRequest);
        supported = false;
    }
    return supported;
}

static void interface_request(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                              size_t req_size) {
    /* Let device handle interface requests */
    usb_device->control_data_callback(setup, buffer, req_size);
}
