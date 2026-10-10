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

/**
 * @file usbip_protocol.h
 * @brief The USB/IP protocol, as specified in the Linux kernel's
 * Documentation/usb/usbip_protocol.rst
 *
 * A connection starts with an operation, OP_REQ_DEVLIST to list the device or OP_REQ_IMPORT
 * to attach it. After an import the connection carries URBs: USBIP_CMD_SUBMIT and
 * USBIP_CMD_UNLINK from the client, USBIP_RET_SUBMIT and USBIP_RET_UNLINK from the server.
 */

#ifndef USBIP_PROTOCOL_H_
#define USBIP_PROTOCOL_H_

#include <stdbool.h>

#include "usbip_device.h"

/**
 * @brief Handle one operation from a client that has not imported the device
 *
 * @return true to disconnect: after a device list, an error or a failed import. false
 *         after a successful import, the connection then carries URBs.
 */
bool usbip_protocol_process_cmd(const usbip_device_t *usb_device);

/**
 * @brief Serve an imported device for a while
 *
 * Waits for a command from the client or for IN data from the device, handles the command
 * and completes pending transfers that are due. Returns at the latest after 500 ms, so the
 * caller can check whether to stop.
 *
 * @return true to disconnect, when the connection was lost
 */
bool usbip_protocol_process_imported_device(const usbip_device_t *usb_device);

/**
 * @return true while a client has the device imported
 */
bool usbip_protocol_is_device_imported(void);

#endif
