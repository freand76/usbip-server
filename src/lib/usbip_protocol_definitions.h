/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2025

 SPDX-License-Identifier: MIT
 See the LICENSE file in the repository.

 code repository located at:
        https://github.com/freand76/usbip-server
********************************************************/

/**
 * @file usbip_protocol_definitions.h
 * @brief Messages of the USB/IP protocol, as they appear on the wire
 *
 * All fields are big endian (network byte order), except the USB setup packet, which keeps
 * the little endian byte order of USB.
 */

#ifndef USBIP_PROTOCOL_DEFINITIONS_H_
#define USBIP_PROTOCOL_DEFINITIONS_H_

#include <stdint.h>

/** @name Byte order conversion between the host, assumed little endian, and the wire
 * @{ */
#define SWAP16(X) __builtin_bswap16(X)
#define SWAP32(X) __builtin_bswap32(X)
/** @} */

/** @name Operations, before a device is imported
 * @{ */
#define OP_REQ_DEVLIST 0x8005
#define OP_REP_DEVLIST 0x0005
#define OP_REQ_IMPORT 0x8003
#define OP_REP_IMPORT 0x0003
/** @} */

/** @name Commands, after a device is imported
 * @{ */
#define USBIP_CMD_SUBMIT 0x00000001
#define USBIP_CMD_UNLINK 0x00000002
#define USBIP_RET_SUBMIT 0x00000003
#define USBIP_RET_UNLINK 0x00000004
/** @} */

/** @brief The exported device, sent with OP_REP_DEVLIST and OP_REP_IMPORT */
typedef struct {
        char path[256];
        char bus_id[32];
        uint32_t busnum;
        uint32_t devnum;
        uint32_t speed;
} __attribute__((packed)) usbip_device_id_t;

/** @brief The device descriptor fields of the device, follows usbip_device_id_t */
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

/** @brief One interface of the device, OP_REP_DEVLIST sends one per interface */
typedef struct {
        uint8_t bInterfaceClass;
        uint8_t bInterfaceSubClass;
        uint8_t bInterfaceProtocol;
        uint8_t padding;
} __attribute__((packed)) usbip_interface_descriptor_t;

/** @brief Header of an operation request */
typedef struct {
        uint16_t version;
        uint16_t command_code;
        uint32_t status;
} __attribute__((packed)) op_req_head_t;

/** @brief Header of an operation reply */
typedef struct {
        uint16_t version;
        uint16_t reply_code;
        uint32_t status;
} __attribute__((packed)) op_rep_head_t;

/** @brief Header shared by all commands and replies */
typedef struct {
        uint32_t command;
        uint32_t seqnum;
        uint32_t devid;
        uint32_t direction;
        uint32_t ep;
} __attribute__((packed)) usbip_header_basic_t;

/**
 * @brief USBIP_CMD_SUBMIT after the basic header
 *
 * Followed by the transfer buffer for OUT transfers, and for isochronous transfers by
 * number_of_packets usbip_iso_packet_descriptor_t.
 */
typedef struct {
        uint32_t transfer_flags;
        uint32_t transfer_buffer_length;
        uint32_t start_frame;
        uint32_t number_of_packets;
        uint32_t interval;
        uint8_t setup[8];
} __attribute__((packed)) usbip_cmd_submit_t;

/**
 * @brief USBIP_RET_SUBMIT after the basic header
 *
 * Followed by actual_length bytes of data for IN transfers, isochronous packets back to back
 * without the gaps between them, and for isochronous transfers by number_of_packets
 * usbip_iso_packet_descriptor_t.
 */
typedef struct {
        uint32_t status;
        uint32_t actual_length;
        uint32_t start_frame;
        uint32_t number_of_packets;
        uint32_t error_count;
        uint64_t padding;
} __attribute__((packed)) usbip_ret_submit_t;

/** @brief One packet of an isochronous transfer */
typedef struct {
        uint32_t offset;
        uint32_t length;
        uint32_t actual_length;
        uint32_t status;
} __attribute__((packed)) usbip_iso_packet_descriptor_t;

/** @brief USBIP_CMD_UNLINK after the basic header, cancels the transfer unlink_seqnum */
typedef struct {
        uint32_t unlink_seqnum;
        uint8_t padding[24];
} __attribute__((packed)) usbip_cmd_unlink_t;

/** @brief USBIP_RET_UNLINK after the basic header, status -ECONNRESET if it was cancelled */
typedef struct {
        uint32_t status;
        uint8_t padding[24];
} __attribute__((packed)) usbip_ret_unlink_t;

#endif
