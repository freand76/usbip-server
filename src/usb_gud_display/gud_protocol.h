/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2026, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

/*
 * The parts of the GUD (Generic USB Display) protocol this device uses. The protocol is
 * specified by the Linux kernel header include/drm/gud.h.
 */

#ifndef GUD_PROTOCOL_H_
#define GUD_PROTOCOL_H_

#include <stdint.h>

/* Protocol values are little endian and copied as they are */
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "The GUD display example needs a little endian host"
#endif

#define GUD_DISPLAY_MAGIC (0x1d50614d)
#define GUD_PROTOCOL_VERSION (1)

#define GUD_DISPLAY_FLAG_STATUS_ON_SET (1 << 0)
#define GUD_DISPLAY_FLAG_FULL_UPDATE (1 << 1)

#define GUD_COMPRESSION_LZ4 (1 << 0)

// Control requests, vendor requests to the interface
#define GUD_REQ_GET_STATUS (0x00)
#define GUD_REQ_GET_DESCRIPTOR (0x01)
#define GUD_REQ_GET_FORMATS (0x40)
#define GUD_REQ_GET_PROPERTIES (0x41)
#define GUD_REQ_GET_CONNECTORS (0x50)
#define GUD_REQ_GET_CONNECTOR_PROPERTIES (0x51)
#define GUD_REQ_GET_CONNECTOR_TV_MODE_VALUES (0x52)
#define GUD_REQ_SET_CONNECTOR_FORCE_DETECT (0x53)
#define GUD_REQ_GET_CONNECTOR_STATUS (0x54)
#define GUD_REQ_GET_CONNECTOR_MODES (0x55)
#define GUD_REQ_GET_CONNECTOR_EDID (0x56)
#define GUD_REQ_SET_BUFFER (0x60)
#define GUD_REQ_SET_STATE_CHECK (0x61)
#define GUD_REQ_SET_STATE_COMMIT (0x62)
#define GUD_REQ_SET_CONTROLLER_ENABLE (0x63)
#define GUD_REQ_SET_DISPLAY_ENABLE (0x64)

// Result of the last request, read with GUD_REQ_GET_STATUS
#define GUD_STATUS_OK (0x00)
#define GUD_STATUS_BUSY (0x01)
#define GUD_STATUS_REQUEST_NOT_SUPPORTED (0x02)
#define GUD_STATUS_PROTOCOL_ERROR (0x03)
#define GUD_STATUS_INVALID_PARAMETER (0x04)
#define GUD_STATUS_ERROR (0x05)

#define GUD_PIXEL_FORMAT_R1 (0x01)
#define GUD_PIXEL_FORMAT_R8 (0x08)
#define GUD_PIXEL_FORMAT_XRGB1111 (0x20)
#define GUD_PIXEL_FORMAT_RGB332 (0x30)
#define GUD_PIXEL_FORMAT_RGB565 (0x40)
#define GUD_PIXEL_FORMAT_RGB888 (0x50)
#define GUD_PIXEL_FORMAT_XRGB8888 (0x80)
#define GUD_PIXEL_FORMAT_ARGB8888 (0x81)

#define GUD_PROPERTY_ROTATION (50)
#define GUD_ROTATION_0 (1 << 0)

#define GUD_CONNECTOR_TYPE_PANEL (0)
#define GUD_CONNECTOR_FLAGS_POLL_STATUS (1 << 0)
#define GUD_CONNECTOR_STATUS_DISCONNECTED (0x00)
#define GUD_CONNECTOR_STATUS_CONNECTED (0x01)

#define GUD_DISPLAY_MODE_FLAG_PREFERRED (1 << 10)

typedef struct {
        uint32_t magic;
        uint8_t version;
        uint32_t flags;
        uint8_t compression;
        uint32_t max_buffer_size;
        uint32_t min_width;
        uint32_t max_width;
        uint32_t min_height;
        uint32_t max_height;
} __attribute__((packed)) gud_display_descriptor_req_t;

typedef struct {
        uint16_t prop;
        uint64_t val;
} __attribute__((packed)) gud_property_req_t;

typedef struct {
        uint32_t clock; /* kHz */
        uint16_t hdisplay;
        uint16_t hsync_start;
        uint16_t hsync_end;
        uint16_t htotal;
        uint16_t vdisplay;
        uint16_t vsync_start;
        uint16_t vsync_end;
        uint16_t vtotal;
        uint32_t flags;
} __attribute__((packed)) gud_display_mode_req_t;

typedef struct {
        uint8_t connector_type;
        uint32_t flags;
} __attribute__((packed)) gud_connector_descriptor_req_t;

/* Sent before each bulk transfer, the rectangle that the transfer updates */
typedef struct {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        uint32_t length;
        uint8_t compression;
        uint32_t compressed_length;
} __attribute__((packed)) gud_set_buffer_req_t;

/* Followed by an array of gud_property_req_t */
typedef struct {
        gud_display_mode_req_t mode;
        uint8_t format;
        uint8_t connector;
} __attribute__((packed)) gud_state_req_t;

#endif
