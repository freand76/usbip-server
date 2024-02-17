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
#include <fcntl.h>

#include <linux/usb/ch9.h>

#include "tcpip_server.h"
#include "usbip_device.h"
#include "usbip_protocol.h"
#include "usbip_protocol_definitions.h"

#define USBIP_MAX_PACKET_SIZE (4096)
#define USBIP_PROTOCOL_VERSION (0x0111)
#define EP_STALL (-32)

static bool device_imported = false;

static uint8_t rx_buffer[USBIP_MAX_PACKET_SIZE];
static uint8_t tx_buffer[USBIP_MAX_PACKET_SIZE];

static size_t get_next_interface_idx(const uint8_t *config_ptr, size_t start_idx);
static bool transmit_response_head(uint16_t response_code);
static bool transmit_number_of_devices(uint32_t num_devices);
static bool transmit_device_id(void);
static bool transmit_device(const usbip_device_t *usb_device);
static void transmit_devlist_response(const usbip_device_t *usb_device);
static bool transmit_import_response(const usbip_device_t *usb_device);

bool usbip_protocol_process_cmd(const usbip_device_t *usb_device) {
    op_req_head_t req_head;
    char busid_str[32] = {0};
    bool status = tcpip_server_read(&req_head, sizeof(req_head));
    if (status) {
        status = (SWAP16(req_head.version) == USBIP_PROTOCOL_VERSION);
    }

    if (status) {
        uint16_t cmd = SWAP16(req_head.command_code);

        switch (cmd) {
        case OP_REQ_DEVLIST:
            printf("USBIPProtocol: USBIP_CMD_REQ_DEV_LIST\n");
            transmit_devlist_response(usb_device);
            break;
        case OP_REQ_IMPORT:
            status = tcpip_server_read(busid_str, sizeof(busid_str));
            if (status) {
                printf("USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '%s'\n", busid_str);
                device_imported = transmit_import_response(usb_device);
            }
            break;
        default:
            status = false;
            break;
        }
    }

    bool disconnect = true;

    if (status && device_imported) {
        disconnect = false;
    }

    return disconnect;
}

bool usbip_protocol_process_imported_device(const usbip_device_t *usb_device) {
    usbip_header_basic_t cmd_header;
    bool status = tcpip_server_read(&cmd_header, sizeof(cmd_header));
    uint32_t command = SWAP32(cmd_header.command);
    bool disconnect = false;
    if (command == 0x00000001) {
        /* SUBMIT */
        usbip_cmd_submit_t cmd_submit;
        status = tcpip_server_read(&cmd_submit, sizeof(cmd_submit));

        uint32_t ep = SWAP32(cmd_header.ep);
        uint32_t direction = SWAP32(cmd_header.direction);
        uint32_t transfer_buffer_length = SWAP32(cmd_submit.transfer_buffer_length);

        size_t tx_length = 0;
        if (direction == 0) {
            status = tcpip_server_read(rx_buffer, transfer_buffer_length);
        }

        if (ep == 0) {
            // Control
            usbip_device_control(usb_device, cmd_submit.setup, rx_buffer, transfer_buffer_length);
        } else {
            if ((direction == 0) && (transfer_buffer_length > 0)) {
                usb_device->ep_data_callback(ep, rx_buffer, transfer_buffer_length);
            }
        }

        uint32_t ret_status = 0;
        {
            uint8_t *ep_buf = usbip_device_get_ep_buffer(ep, &tx_length);
            if (tx_length > 0) {
                memcpy(tx_buffer, ep_buf, tx_length);
                usbip_device_release_buffer(ep);
            } else {
                // ret_status = EP_STALL;
            }
        }

        usbip_header_basic_t ret_header = {0};
        ret_header.command = SWAP32(0x00000003);
        ret_header.seqnum = cmd_header.seqnum;
        ret_header.devid = SWAP32(0);
        ret_header.direction = SWAP32(0);
        ret_header.ep = SWAP32(0);
        if (status) {
            status = tcpip_server_write(&ret_header, sizeof(ret_header));
        }

        usbip_ret_submit_t ret_submit = {0};
        ret_submit.status = SWAP32(ret_status);
        ret_submit.actual_length = SWAP32(tx_length);
        ret_submit.start_frame = SWAP32(0);
        ret_submit.number_of_packets = SWAP32(0xffffffff);
        ret_submit.error_count = SWAP32(0);

        status = tcpip_server_write(&ret_submit, sizeof(ret_submit));
        if (status && (tx_length > 0)) {
            status = tcpip_server_write(tx_buffer, tx_length);
        }

    } else if (command == 0x00000002) {
        /* UNLINK */
        usbip_cmd_unlink_t cmd_unlink;
        status = tcpip_server_read(&cmd_unlink, sizeof(cmd_unlink));

        usbip_header_basic_t ret_header = {0};
        ret_header.command = SWAP32(0x00000004);
        ret_header.seqnum = cmd_unlink.unlink_seqnum;
        ret_header.devid = SWAP32(0);
        ret_header.direction = SWAP32(0);
        ret_header.ep = SWAP32(0);
        if (status) {
            status = tcpip_server_write(&ret_header, sizeof(ret_header));
        }

        usbip_ret_unlink_t ret_unlink = {0};
        ret_unlink.status = SWAP32(0);
        if (status) {
            status = tcpip_server_write(&ret_unlink, sizeof(ret_unlink));
        }

    } else {
        status = false;
    }

    if (!status) {
        printf("USBIPProtocol: Lost connection :-(\n");
        disconnect = true;
        device_imported = false;
    }

    return disconnect;
}

bool usbip_protocol_is_device_imported(void) {
    return device_imported;
}

static size_t get_next_interface_idx(const uint8_t *config_ptr, size_t start_idx) {
    uint16_t total_size = (config_ptr[3] << 8) | config_ptr[2];
    uint8_t desc_size = config_ptr[start_idx];
    size_t idx = start_idx;
    while (idx < total_size) {
        idx += desc_size;
        if (config_ptr[idx + 1] == 0x04) {
            break;
        }
        desc_size = config_ptr[idx];
    }
    return idx;
}

static bool transmit_response_head(uint16_t response_code) {
    op_rep_head_t rep_head = {0};
    rep_head.version = SWAP16(USBIP_PROTOCOL_VERSION);
    rep_head.reply_code = SWAP16(response_code);
    rep_head.status = SWAP32(0);
    return tcpip_server_write(&rep_head, sizeof(rep_head));
}

static bool transmit_number_of_devices(uint32_t num_devices) {
    uint32_t number_of_devices = SWAP32(num_devices);
    return tcpip_server_write(&number_of_devices, sizeof(number_of_devices));
}

static bool transmit_device_id(void) {
    usbip_device_id_t device_id = {0};
    strcpy(device_id.path, "sys/usb/virtual_device");
    strcpy(device_id.bus_id, "1-1");
    device_id.busnum = SWAP32(2);
    device_id.devnum = SWAP32(3);
    device_id.speed = SWAP32(USB_SPEED_HIGH);
    return tcpip_server_write(&device_id, sizeof(device_id));
}

static bool transmit_device(const usbip_device_t *usb_device) {
    usbip_device_descriptor_t device = {0};
    memcpy(&device.idVendor, &usb_device->device_desc[8], sizeof(uint16_t));
    memcpy(&device.idProduct, &usb_device->device_desc[10], sizeof(uint16_t));
    memcpy(&device.bcdDevice, &usb_device->device_desc[12], sizeof(uint16_t));
    memcpy(&device.bDeviceClass, &usb_device->device_desc[4], sizeof(uint8_t));
    memcpy(&device.bDeviceSubClass, &usb_device->device_desc[6], sizeof(uint8_t));
    memcpy(&device.bDeviceProtocol, &usb_device->device_desc[8], sizeof(uint8_t));
    device.bConfigurationValue = usb_device->config_desc[5];
    device.bNumConfigurations = usb_device->device_desc[17];
    device.bNumInterfaces = usb_device->config_desc[4];

    return tcpip_server_write(&device, sizeof(device));
}

static void transmit_devlist_response(const usbip_device_t *usb_device) {
    bool status = transmit_response_head(OP_REP_DEVLIST);
    if (status) {
        status = transmit_number_of_devices(1);
    }
    if (status) {
        status = transmit_device_id();
    }
    if (status) {
        status = transmit_device(usb_device);
    }

    size_t iface_idx = 0;
    uint8_t num_interfaces = usb_device->config_desc[4];
    for (uint8_t idx = 0; idx < num_interfaces; idx++) {
        iface_idx = get_next_interface_idx(usb_device->config_desc, iface_idx);
        usbip_interface_descriptor_t interface = {0};
        interface.bInterfaceClass = usb_device->config_desc[iface_idx + 5];
        interface.bInterfaceSubClass = usb_device->config_desc[iface_idx + 6];
        interface.bInterfaceProtocol = usb_device->config_desc[iface_idx + 7];

        if (status) {
            status = tcpip_server_write(&interface, sizeof(interface));
        }
    }
}

static bool transmit_import_response(const usbip_device_t *usb_device) {
    bool status = transmit_response_head(OP_REP_IMPORT);
    if (status) {
        status = transmit_device_id();
    }
    if (status) {
        status = transmit_device(usb_device);
    }
    return status;
}
