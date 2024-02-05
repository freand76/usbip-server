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

#ifndef USBIP_PROTOCOL_DEFINITIONS_H_
#define USBIP_PROTOCOL_DEFINITIONS_H_

#include <stdint.h>

#define SWAP16(X) __builtin_bswap16(X)
#define SWAP32(X) __builtin_bswap32(X)

#define OP_REQ_DEVLIST 0x8005
#define OP_REP_DEVLIST 0x0005
#define OP_REQ_IMPORT 0x8003
#define OP_REP_IMPORT 0x0003

typedef struct {
        char path[256];
        char bus_id[32];
        uint32_t busnum;
        uint32_t devnum;
        uint32_t speed;
} __attribute__((packed)) usbip_device_id_t;

typedef struct {
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bConfigurationValue;
        uint8_t bNumConfigurations;
        uint8_t bNumInterfaces;
} __attribute__((packed)) usbip_device_descriptor_t;

typedef struct {
        uint8_t bInterfaceClass;
        uint8_t bInterfaceSubClass;
        uint8_t bInterfaceProtocol;
        uint8_t padding;
} __attribute__((packed)) usbip_interface_descriptor_t;

typedef struct {
        uint16_t version;
        uint16_t command_code;
        uint32_t status;
} __attribute__((packed)) op_req_head_t;

typedef struct {
        uint16_t version;
        uint16_t reply_code;
        uint32_t status;
} __attribute__((packed)) op_rep_head_t;

typedef struct {
        uint32_t command;
        uint32_t seqnum;
        uint32_t devid;
        uint32_t direction;
        uint32_t ep;
} __attribute__((packed)) usbip_header_basic_t;

typedef struct {
        uint32_t transfer_flags;
        uint32_t transfer_buffer_length;
        uint32_t start_frame;
        uint32_t number_of_packets;
        uint32_t interval;
        uint8_t setup[8];
} __attribute__((packed)) usbip_cmd_submit_t;

typedef struct {
        uint32_t status;
        uint32_t actual_length;
        uint32_t start_frame;
        uint32_t number_of_packets;
        uint32_t error_count;
        uint64_t padding;
} __attribute__((packed)) usbip_ret_submit_t;

typedef struct {
        uint32_t unlink_seqnum;
        uint8_t padding[24];
} __attribute__((packed)) usbip_cmd_unlink_t;

typedef struct {
        uint32_t status;
        uint8_t padding[24];
} __attribute__((packed)) usbip_ret_unlink_t;

#endif
