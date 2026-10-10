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
#include <fcntl.h>

#include <linux/usb/ch9.h>

#include "tcpip_server.h"
#include "usbip_device.h"
#include "usbip_protocol.h"
#include "usbip_protocol_definitions.h"

#define USBIP_MAX_PACKET_SIZE (4096)
#define USBIP_PROTOCOL_VERSION (0x0111)
#define EP_STALL (-32)
#define URB_UNLINKED (-104) /* -ECONNRESET */
#define MAX_PENDING_URBS (32)

typedef struct {
        uint32_t seqnum;
        uint32_t ep;
        uint32_t transfer_buffer_length;
} pending_urb_t;

static bool device_imported = false;

static pending_urb_t pending_urbs[MAX_PENDING_URBS];
static size_t num_pending_urbs = 0;

static uint8_t rx_buffer[USBIP_MAX_PACKET_SIZE];
static uint8_t tx_buffer[USBIP_MAX_PACKET_SIZE];

static size_t get_next_interface_idx(const uint8_t *config_ptr, size_t start_idx);
static bool transmit_response_head(uint16_t response_code);
static bool transmit_number_of_devices(uint32_t num_devices);
static bool transmit_device_id(void);
static bool transmit_device(const usbip_device_t *usb_device);
static bool transmit_devlist_response(const usbip_device_t *usb_device);
static bool transmit_import_response(const usbip_device_t *usb_device);
static bool process_imported_cmd(const usbip_device_t *usb_device);
static bool process_submit(const usbip_device_t *usb_device, uint32_t seqnum, uint32_t ep,
                           uint32_t direction);
static bool process_unlink(uint32_t seqnum);
static bool complete_pending_urbs(void);
static void remove_pending_urb(size_t idx);
static bool transmit_ret_submit(uint32_t seqnum, size_t tx_length);

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
            status = transmit_devlist_response(usb_device);
            break;
        case OP_REQ_IMPORT:
            status = tcpip_server_read(busid_str, sizeof(busid_str));
            if (status) {
                printf("USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '%s'\n", busid_str);
                num_pending_urbs = 0;
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
    bool status = true;

    if (tcpip_server_wait_for_data(usbip_device_get_event_fd())) {
        status = process_imported_cmd(usb_device);
    }

    /* Clear before completing, data transmitted after this wakes the next wait */
    usbip_device_clear_event();
    if (status) {
        status = complete_pending_urbs();
    }

    if (!status) {
        printf("USBIPProtocol: Lost connection :-(\n");
        device_imported = false;
        num_pending_urbs = 0;
    }

    // Return true for disconnect
    return status ? false : true;
}

bool usbip_protocol_is_device_imported(void) {
    return device_imported;
}

static bool process_imported_cmd(const usbip_device_t *usb_device) {
    usbip_header_basic_t cmd_header;
    bool status = tcpip_server_read(&cmd_header, sizeof(cmd_header));

    if (status) {
        uint32_t command = SWAP32(cmd_header.command);
        uint32_t seqnum = SWAP32(cmd_header.seqnum);
        if (command == USBIP_CMD_SUBMIT) {
            status = process_submit(usb_device, seqnum, SWAP32(cmd_header.ep),
                                    SWAP32(cmd_header.direction));
        } else if (command == USBIP_CMD_UNLINK) {
            status = process_unlink(seqnum);
        } else {
            status = false;
        }
    }
    return status;
}

static bool process_submit(const usbip_device_t *usb_device, uint32_t seqnum, uint32_t ep,
                           uint32_t direction) {
    usbip_cmd_submit_t cmd_submit;
    bool status = tcpip_server_read(&cmd_submit, sizeof(cmd_submit));
    uint32_t transfer_buffer_length = SWAP32(cmd_submit.transfer_buffer_length);

    if (status && (transfer_buffer_length > USBIP_MAX_PACKET_SIZE)) {
        fprintf(stderr, "USBIPProtocol: Transfer buffer length %u exceeds max packet size %u\n",
                transfer_buffer_length, USBIP_MAX_PACKET_SIZE);
        status = false;
    }

    if (status && (direction == 0)) {
        status = tcpip_server_read(rx_buffer, transfer_buffer_length);
    }

    if (!status) {
        return false;
    }

    if (ep == 0) {
        // Control
        usbip_device_control(usb_device, cmd_submit.setup, rx_buffer, transfer_buffer_length);
        size_t tx_length = 0;
        if (direction == 1) {
            tx_length = usbip_device_fetch_ep_data(0, tx_buffer, transfer_buffer_length);
        }
        return transmit_ret_submit(seqnum, tx_length);
    }

    if (direction == 0) {
        if ((transfer_buffer_length > 0) && (usb_device->ep_data_callback != NULL)) {
            usbip_device_receive(usb_device, ep, rx_buffer, transfer_buffer_length);
        }
        return transmit_ret_submit(seqnum, 0);
    }

    /* IN transfers stay pending until the device has data, like a NAK on a real bus */
    if (num_pending_urbs == MAX_PENDING_URBS) {
        fprintf(stderr, "USBIPProtocol: Too many pending transfers, completing %u without data\n",
                seqnum);
        return transmit_ret_submit(seqnum, 0);
    }
    pending_urbs[num_pending_urbs++] = (pending_urb_t){
        .seqnum = seqnum,
        .ep = ep,
        .transfer_buffer_length = transfer_buffer_length,
    };
    return true;
}

static bool process_unlink(uint32_t seqnum) {
    usbip_cmd_unlink_t cmd_unlink;
    bool status = tcpip_server_read(&cmd_unlink, sizeof(cmd_unlink));
    if (!status) {
        return false;
    }

    /* A transfer that already completed can not be unlinked */
    int32_t unlink_status = 0;
    uint32_t unlink_seqnum = SWAP32(cmd_unlink.unlink_seqnum);
    for (size_t idx = 0; idx < num_pending_urbs; idx++) {
        if (pending_urbs[idx].seqnum == unlink_seqnum) {
            remove_pending_urb(idx);
            unlink_status = URB_UNLINKED;
            break;
        }
    }

    usbip_header_basic_t ret_header = {0};
    ret_header.command = SWAP32(USBIP_RET_UNLINK);
    ret_header.seqnum = SWAP32(seqnum);
    ret_header.devid = SWAP32(0);
    ret_header.direction = SWAP32(0);
    ret_header.ep = SWAP32(0);
    status = tcpip_server_write(&ret_header, sizeof(ret_header));

    if (status) {
        usbip_ret_unlink_t ret_unlink = {0};
        ret_unlink.status = SWAP32((uint32_t)unlink_status);
        status = tcpip_server_write(&ret_unlink, sizeof(ret_unlink));
    }
    return status;
}

static bool complete_pending_urbs(void) {
    bool status = true;
    size_t idx = 0;
    /* Oldest first, so data reaches the host in the order the transfers were queued */
    while (status && (idx < num_pending_urbs)) {
        pending_urb_t urb = pending_urbs[idx];
        size_t tx_length =
            usbip_device_fetch_ep_data(urb.ep, tx_buffer, urb.transfer_buffer_length);
        if (tx_length > 0) {
            remove_pending_urb(idx);
            status = transmit_ret_submit(urb.seqnum, tx_length);
        } else {
            idx++;
        }
    }
    return status;
}

static void remove_pending_urb(size_t idx) {
    num_pending_urbs--;
    memmove(&pending_urbs[idx], &pending_urbs[idx + 1],
            (num_pending_urbs - idx) * sizeof(pending_urbs[0]));
}

static bool transmit_ret_submit(uint32_t seqnum, size_t tx_length) {
    usbip_header_basic_t ret_header = {0};
    ret_header.command = SWAP32(USBIP_RET_SUBMIT);
    ret_header.seqnum = SWAP32(seqnum);
    ret_header.devid = SWAP32(0);
    ret_header.direction = SWAP32(0);
    ret_header.ep = SWAP32(0);
    bool status = tcpip_server_write(&ret_header, sizeof(ret_header));

    if (status) {
        usbip_ret_submit_t ret_submit = {0};
        ret_submit.status = SWAP32(0);
        ret_submit.actual_length = SWAP32(tx_length);
        ret_submit.start_frame = SWAP32(0);
        ret_submit.number_of_packets = SWAP32(0xffffffff);
        ret_submit.error_count = SWAP32(0);
        status = tcpip_server_write(&ret_submit, sizeof(ret_submit));
    }

    if (status && (tx_length > 0)) {
        status = tcpip_server_write(tx_buffer, tx_length);
    }
    return status;
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
    device.idVendor =
        SWAP16((uint16_t)((usb_device->device_desc[9] << 8) | usb_device->device_desc[8]));
    device.idProduct =
        SWAP16((uint16_t)((usb_device->device_desc[11] << 8) | usb_device->device_desc[10]));
    device.bcdDevice =
        SWAP16((uint16_t)((usb_device->device_desc[13] << 8) | usb_device->device_desc[12]));
    memcpy(&device.bDeviceClass, &usb_device->device_desc[4], sizeof(uint8_t));
    memcpy(&device.bDeviceSubClass, &usb_device->device_desc[5], sizeof(uint8_t));
    memcpy(&device.bDeviceProtocol, &usb_device->device_desc[6], sizeof(uint8_t));
    device.bConfigurationValue = usb_device->config_desc[5];
    device.bNumConfigurations = usb_device->device_desc[17];
    device.bNumInterfaces = usb_device->config_desc[4];

    return tcpip_server_write(&device, sizeof(device));
}

static bool transmit_devlist_response(const usbip_device_t *usb_device) {
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

        if (!status) {
            // Early exit for connection problems
            break;
        }
    }
    return status;
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
