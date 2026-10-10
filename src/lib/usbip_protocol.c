/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2026, All Rights Reserved.

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
#include <time.h>

#include <linux/usb/ch9.h>

#include "tcpip_server.h"
#include "usbip_device.h"
#include "usbip_protocol.h"
#include "usbip_protocol_definitions.h"

#define USBIP_MAX_PACKET_SIZE (64 * 1024)
#define USBIP_PROTOCOL_VERSION (0x0111)
#define EP_STALL (-32)
#define URB_UNLINKED (-104) /* -ECONNRESET */
#define MAX_PENDING_URBS (32)
/* The most packets usbfs allows in one isochronous transfer, kernel drivers use fewer */
#define MAX_ISO_PACKETS (128)
#define NUMBER_OF_ENDPOINTS (16)
#define WAIT_TIMEOUT_US (500000)

typedef struct {
        uint8_t type;           /* USB_ENDPOINT_XFER_* from the endpoint descriptor */
        uint64_t interval_us;   /* Service interval of interrupt IN and isochronous endpoints */
        uint64_t next_poll_us;  /* Interrupt IN: when the next transfer may complete */
        uint64_t stream_end_us; /* Isochronous: when the last queued transfer completes */
} endpoint_t;

typedef struct {
        uint32_t offset;
        uint32_t length;
} iso_packet_t;

typedef struct {
        uint32_t seqnum;
        uint32_t ep;
        uint32_t direction;
        uint32_t transfer_buffer_length;
        /* Isochronous transfers only */
        bool isochronous;
        uint32_t start_frame;
        uint32_t number_of_packets;
        uint64_t complete_us;
        iso_packet_t packets[MAX_ISO_PACKETS];
} pending_urb_t;

static bool device_imported = false;

static pending_urb_t pending_urbs[MAX_PENDING_URBS];
static size_t num_pending_urbs = 0;

/* Indexed by direction (0 = OUT, 1 = IN) and endpoint number, interrupt IN and isochronous
   endpoints complete transfers at their service interval, like on a real bus */
static endpoint_t endpoints[2][NUMBER_OF_ENDPOINTS];
static uint64_t wait_timeout_us = WAIT_TIMEOUT_US;

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
static bool process_iso_submit(const usbip_device_t *usb_device, uint32_t seqnum, uint32_t ep,
                               uint32_t direction, const usbip_cmd_submit_t *cmd_submit,
                               endpoint_t *endpoint);
static bool process_unlink(uint32_t seqnum);
static bool complete_pending_urbs(const usbip_device_t *usb_device);
static bool has_pending_iso_urbs(uint32_t ep, uint32_t direction);
static void load_endpoints(const uint8_t *config_desc);
static uint64_t now_us(void);
static void remove_pending_urb(size_t idx);
static bool transmit_ret_submit(uint32_t seqnum, int32_t urb_status, size_t actual_length,
                                bool send_data);
static bool transmit_iso_ret_submit(const usbip_device_t *usb_device, const pending_urb_t *urb);

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
                /* The client is not required to NUL terminate the bus id */
                printf("USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '%.*s'\n", (int)sizeof(busid_str),
                       busid_str);
                /* A new host starts with a freshly reset device */
                num_pending_urbs = 0;
                wait_timeout_us = WAIT_TIMEOUT_US;
                load_endpoints(usb_device->config_desc);
                usbip_device_reset(usb_device);
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

    if (tcpip_server_wait_for_data(usbip_device_get_event_fd(), wait_timeout_us)) {
        status = process_imported_cmd(usb_device);
    }

    /* Clear before completing, data transmitted after this wakes the next wait */
    usbip_device_clear_event();
    if (status) {
        status = complete_pending_urbs(usb_device);
    }

    if (!status) {
        printf("USBIPProtocol: Lost connection :-(\n");
        device_imported = false;
        num_pending_urbs = 0;
        usbip_device_reset(usb_device);
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
    endpoint_t *endpoint = &endpoints[(direction != 0) ? 1 : 0][ep & USB_ENDPOINT_NUMBER_MASK];

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

    if ((ep != 0) && (endpoint->type == USB_ENDPOINT_XFER_ISOC)) {
        return process_iso_submit(usb_device, seqnum, ep, direction, &cmd_submit, endpoint);
    }

    if (ep == 0) {
        // Control
        bool supported =
            usbip_device_control(usb_device, cmd_submit.setup, rx_buffer, transfer_buffer_length);
        size_t tx_length = 0;
        if (direction == 1) {
            tx_length = usbip_device_fetch_ep_data(0, tx_buffer, transfer_buffer_length);
            /* A device that has no data for an IN request stalls it */
            if ((transfer_buffer_length > 0) && (tx_length == 0)) {
                supported = false;
            }
        }
        if (!supported) {
            return transmit_ret_submit(seqnum, EP_STALL, 0, false);
        }
        if (direction == 0) {
            return transmit_ret_submit(seqnum, 0, transfer_buffer_length, false);
        }
        return transmit_ret_submit(seqnum, 0, tx_length, true);
    }

    if (direction == 0) {
        if ((transfer_buffer_length > 0) && (usb_device->ep_data_callback != NULL)) {
            usbip_device_receive(usb_device, ep, rx_buffer, transfer_buffer_length);
        }
        /* The host learns how much of its OUT data was accepted, all of it */
        return transmit_ret_submit(seqnum, 0, transfer_buffer_length, false);
    }

    /* IN transfers stay pending until the device has data, like a NAK on a real bus */
    if (num_pending_urbs == MAX_PENDING_URBS) {
        fprintf(stderr, "USBIPProtocol: Too many pending transfers, completing %u without data\n",
                seqnum);
        return transmit_ret_submit(seqnum, 0, 0, true);
    }
    pending_urb_t *urb = &pending_urbs[num_pending_urbs++];
    urb->seqnum = seqnum;
    urb->ep = ep;
    urb->direction = direction;
    urb->transfer_buffer_length = transfer_buffer_length;
    urb->isochronous = false;
    return true;
}

static bool process_iso_submit(const usbip_device_t *usb_device, uint32_t seqnum, uint32_t ep,
                               uint32_t direction, const usbip_cmd_submit_t *cmd_submit,
                               endpoint_t *endpoint) {
    static usbip_iso_packet_descriptor_t descriptors[MAX_ISO_PACKETS];
    static pending_urb_t urb;
    uint32_t number_of_packets = SWAP32(cmd_submit->number_of_packets);

    if (number_of_packets > MAX_ISO_PACKETS) {
        fprintf(stderr,
                "USBIPProtocol: %u isochronous packets in one transfer, at most %u supported\n",
                number_of_packets, MAX_ISO_PACKETS);
        return false;
    }
    if (!tcpip_server_read(descriptors, number_of_packets * sizeof(descriptors[0]))) {
        return false;
    }

    urb.seqnum = seqnum;
    urb.ep = ep;
    urb.direction = direction;
    urb.transfer_buffer_length = SWAP32(cmd_submit->transfer_buffer_length);
    urb.isochronous = true;
    urb.start_frame = SWAP32(cmd_submit->start_frame);
    urb.number_of_packets = number_of_packets;
    for (uint32_t idx = 0; idx < number_of_packets; idx++) {
        urb.packets[idx].offset = SWAP32(descriptors[idx].offset);
        urb.packets[idx].length = SWAP32(descriptors[idx].length);
        /* Packets are read from and written to the transfer buffer at these positions */
        if ((uint64_t)urb.packets[idx].offset + urb.packets[idx].length >
            urb.transfer_buffer_length) {
            fprintf(stderr, "USBIPProtocol: Isochronous packet %u is outside the transfer buffer\n",
                    idx);
            return false;
        }
    }

    if (direction == 0) {
        for (uint32_t idx = 0; idx < number_of_packets; idx++) {
            if ((urb.packets[idx].length > 0) && (usb_device->ep_data_callback != NULL)) {
                usbip_device_receive(usb_device, ep, &rx_buffer[urb.packets[idx].offset],
                                     urb.packets[idx].length);
            }
        }
    }

    /* A transfer takes one service interval per packet, and starts when the previous transfer on
       the endpoint ends, so a stream keeps the endpoint's real-time pace */
    uint64_t now = now_us();
    uint64_t start = (endpoint->stream_end_us > now) ? endpoint->stream_end_us : now;
    urb.complete_us = start + (number_of_packets * endpoint->interval_us);
    endpoint->stream_end_us = urb.complete_us;

    if (num_pending_urbs == MAX_PENDING_URBS) {
        fprintf(stderr, "USBIPProtocol: Too many pending transfers, completing %u now\n", seqnum);
        return transmit_iso_ret_submit(usb_device, &urb);
    }
    pending_urbs[num_pending_urbs++] = urb;
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
            uint32_t ep = pending_urbs[idx].ep;
            uint32_t direction = pending_urbs[idx].direction;
            bool isochronous = pending_urbs[idx].isochronous;
            remove_pending_urb(idx);
            unlink_status = URB_UNLINKED;
            /* A stream that was stopped starts over without waiting for its cancelled transfers */
            if (isochronous && !has_pending_iso_urbs(ep, direction)) {
                endpoints[(direction != 0) ? 1 : 0][ep & USB_ENDPOINT_NUMBER_MASK].stream_end_us =
                    0;
            }
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

static bool complete_pending_urbs(const usbip_device_t *usb_device) {
    bool status = true;
    uint64_t now = now_us();
    size_t idx = 0;
    wait_timeout_us = WAIT_TIMEOUT_US;
    /* Oldest first, so data reaches the host in the order the transfers were queued */
    while (status && (idx < num_pending_urbs)) {
        pending_urb_t *urb = &pending_urbs[idx];
        endpoint_t *endpoint = &endpoints[1][urb->ep & USB_ENDPOINT_NUMBER_MASK];
        uint64_t due_us = urb->isochronous ? urb->complete_us : endpoint->next_poll_us;
        if (now < due_us) {
            /* Not polled yet, make sure to wake up when it is */
            if ((due_us - now) < wait_timeout_us) {
                wait_timeout_us = due_us - now;
            }
            idx++;
        } else if (urb->isochronous) {
            status = transmit_iso_ret_submit(usb_device, urb);
            remove_pending_urb(idx);
        } else {
            size_t tx_length =
                usbip_device_fetch_ep_data(urb->ep, tx_buffer, urb->transfer_buffer_length);
            if (tx_length > 0) {
                uint32_t seqnum = urb->seqnum;
                endpoint->next_poll_us = now + endpoint->interval_us;
                remove_pending_urb(idx);
                status = transmit_ret_submit(seqnum, 0, tx_length, true);
            } else {
                idx++;
            }
        }
    }
    return status;
}

static bool has_pending_iso_urbs(uint32_t ep, uint32_t direction) {
    for (size_t idx = 0; idx < num_pending_urbs; idx++) {
        if (pending_urbs[idx].isochronous && (pending_urbs[idx].ep == ep) &&
            (pending_urbs[idx].direction == direction)) {
            return true;
        }
    }
    return false;
}

static void load_endpoints(const uint8_t *config_desc) {
    size_t total_length = (config_desc[3] << 8) | config_desc[2];
    memset(endpoints, 0, sizeof(endpoints));

    size_t idx = 0;
    while ((idx + USB_DT_ENDPOINT_SIZE <= total_length) && (config_desc[idx] > 0)) {
        const uint8_t *desc = &config_desc[idx];
        if (desc[1] == USB_DT_ENDPOINT) {
            bool in = ((desc[2] & USB_DIR_IN) != 0);
            endpoint_t *endpoint = &endpoints[in ? 1 : 0][desc[2] & USB_ENDPOINT_NUMBER_MASK];
            endpoint->type = desc[3] & USB_ENDPOINT_XFERTYPE_MASK;
            bool paced = (endpoint->type == USB_ENDPOINT_XFER_ISOC) ||
                         (in && (endpoint->type == USB_ENDPOINT_XFER_INT));
            if (paced && (desc[6] > 0)) {
                /* The device is reported as high speed, interval is 2^(bInterval - 1) * 125 us */
                uint8_t exponent = (desc[6] > 16) ? 16 : desc[6];
                endpoint->interval_us = (uint64_t)125 << (exponent - 1);
            }
        }
        idx += desc[0];
    }
}

static uint64_t now_us(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ((uint64_t)now.tv_sec * 1000000) + ((uint64_t)now.tv_nsec / 1000);
}

static void remove_pending_urb(size_t idx) {
    num_pending_urbs--;
    memmove(&pending_urbs[idx], &pending_urbs[idx + 1],
            (num_pending_urbs - idx) * sizeof(pending_urbs[0]));
}

/* IN transfers send actual_length bytes of data, OUT transfers only report how much was accepted */
static bool transmit_ret_submit(uint32_t seqnum, int32_t urb_status, size_t actual_length,
                                bool send_data) {
    usbip_header_basic_t ret_header = {0};
    ret_header.command = SWAP32(USBIP_RET_SUBMIT);
    ret_header.seqnum = SWAP32(seqnum);
    ret_header.devid = SWAP32(0);
    ret_header.direction = SWAP32(0);
    ret_header.ep = SWAP32(0);
    bool status = tcpip_server_write(&ret_header, sizeof(ret_header));

    if (status) {
        usbip_ret_submit_t ret_submit = {0};
        ret_submit.status = SWAP32((uint32_t)urb_status);
        ret_submit.actual_length = SWAP32(actual_length);
        ret_submit.start_frame = SWAP32(0);
        ret_submit.number_of_packets = SWAP32(0xffffffff);
        ret_submit.error_count = SWAP32(0);
        status = tcpip_server_write(&ret_submit, sizeof(ret_submit));
    }

    if (status && send_data && (actual_length > 0)) {
        status = tcpip_server_write(tx_buffer, actual_length);
    }
    return status;
}

/* IN packets are sent back to back without the gaps between them, followed by the packet
   descriptors, OUT transfers only report how much of each packet was accepted */
static bool transmit_iso_ret_submit(const usbip_device_t *usb_device, const pending_urb_t *urb) {
    static usbip_iso_packet_descriptor_t descriptors[MAX_ISO_PACKETS];
    size_t total_length = 0;

    for (uint32_t idx = 0; idx < urb->number_of_packets; idx++) {
        size_t actual_length = urb->packets[idx].length;
        if (urb->direction != 0) {
            size_t space = urb->transfer_buffer_length - total_length;
            size_t max_length = (actual_length < space) ? actual_length : space;
            actual_length =
                usbip_device_iso_in(usb_device, urb->ep, &tx_buffer[total_length], max_length);
        }
        total_length += actual_length;
        descriptors[idx].offset = SWAP32(urb->packets[idx].offset);
        descriptors[idx].length = SWAP32(urb->packets[idx].length);
        descriptors[idx].actual_length = SWAP32((uint32_t)actual_length);
        descriptors[idx].status = SWAP32(0);
    }

    usbip_header_basic_t ret_header = {0};
    ret_header.command = SWAP32(USBIP_RET_SUBMIT);
    ret_header.seqnum = SWAP32(urb->seqnum);
    bool status = tcpip_server_write(&ret_header, sizeof(ret_header));

    if (status) {
        usbip_ret_submit_t ret_submit = {0};
        ret_submit.status = SWAP32(0);
        ret_submit.actual_length = SWAP32((uint32_t)total_length);
        ret_submit.start_frame = SWAP32(urb->start_frame);
        ret_submit.number_of_packets = SWAP32(urb->number_of_packets);
        ret_submit.error_count = SWAP32(0);
        status = tcpip_server_write(&ret_submit, sizeof(ret_submit));
    }

    if (status && (urb->direction != 0) && (total_length > 0)) {
        status = tcpip_server_write(tx_buffer, total_length);
    }

    if (status && (urb->number_of_packets > 0)) {
        status = tcpip_server_write(descriptors, urb->number_of_packets * sizeof(descriptors[0]));
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
