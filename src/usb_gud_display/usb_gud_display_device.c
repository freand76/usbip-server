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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "usbip_server_api.h"
#include "gud_display_window.h"
#include "usb_gud_display_descriptors.h"

#define REQUEST_TYPE_VENDOR_INTERFACE_IN (0xC1)
#define REQUEST_TYPE_VENDOR_INTERFACE_OUT (0x41)

#define MAX_BYTES_PER_PIXEL (4)
#define WINDOW_FRAME_INTERVAL_MS (20)
#define CONNECTOR_CONNECT_DELAY_MS (5000)

typedef struct {
        bool active;
        gud_set_buffer_req_t req;
        uint32_t transfer_length; /* Compressed length when compressed */
        uint32_t received;
} pending_buffer_t;

// Only used from device callbacks, which never run at the same time
static uint8_t last_status = GUD_STATUS_OK;
static uint64_t attached_ms = 0;
static bool state_checked = false;
static gud_state_req_t checked_state;
static uint8_t current_format = 0;
static bool controller_enabled = false;
static bool display_enabled = false;
static pending_buffer_t pending;
static uint8_t transfer_buffer[DISPLAY_MAX_BUFFER_SIZE];
static uint8_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * MAX_BYTES_PER_PIXEL];
static bool framebuffer_changed = false;

// Statistics printed by the main loop, shared under the critical section
static uint32_t stats_updates = 0;
static uint32_t stats_bytes = 0;
static uint32_t stats_errors = 0;
static gud_set_buffer_req_t stats_last_rect;

static uint64_t now_ms(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ((uint64_t)now.tv_sec * 1000) + ((uint64_t)now.tv_nsec / 1000000);
}

static bool is_offered_format(uint8_t format) {
    for (size_t idx = 0; idx < sizeof(display_formats); idx++) {
        if (display_formats[idx] == format) {
            return true;
        }
    }
    return false;
}

static uint32_t format_bits_per_pixel(uint8_t format) {
    switch (format) {
    case GUD_PIXEL_FORMAT_R1:
        return 1;
    case GUD_PIXEL_FORMAT_XRGB1111:
        return 4;
    case GUD_PIXEL_FORMAT_R8:
    case GUD_PIXEL_FORMAT_RGB332:
        return 8;
    case GUD_PIXEL_FORMAT_RGB565:
        return 16;
    case GUD_PIXEL_FORMAT_RGB888:
        return 24;
    case GUD_PIXEL_FORMAT_XRGB8888:
    case GUD_PIXEL_FORMAT_ARGB8888:
        return 32;
    default:
        return 0;
    }
}

/* Bytes per line of a rectangle, lines of formats below 8 bits per pixel are byte aligned */
static uint32_t line_length(uint8_t format, uint32_t width) {
    return (width * format_bits_per_pixel(format) + 7) / 8;
}

static void answer(const void *data, size_t length) {
    last_status = GUD_STATUS_OK;
    usbip_device_transmit(0, (const uint8_t *)data, length);
}

static void get_request(uint8_t bRequest, uint16_t wValue) {
    switch (bRequest) {
    case GUD_REQ_GET_STATUS:
        /* Reports the last request, so it must not change the status itself */
        usbip_device_transmit(0, &last_status, sizeof(last_status));
        return;
    case GUD_REQ_GET_DESCRIPTOR: {
        gud_display_descriptor_req_t desc = {
            .magic = GUD_DISPLAY_MAGIC,
            .version = GUD_PROTOCOL_VERSION,
            /* Control OUT requests can not be rejected, so the host asks for their status */
            .flags = GUD_DISPLAY_FLAG_STATUS_ON_SET,
            .compression = DISPLAY_COMPRESSION,
            .max_buffer_size = DISPLAY_MAX_BUFFER_SIZE,
            .min_width = DISPLAY_WIDTH,
            .max_width = DISPLAY_WIDTH,
            .min_height = DISPLAY_HEIGHT,
            .max_height = DISPLAY_HEIGHT,
        };
        answer(&desc, sizeof(desc));
        return;
    }
    case GUD_REQ_GET_FORMATS:
        answer(display_formats, sizeof(display_formats));
        return;
    case GUD_REQ_GET_PROPERTIES: {
        /* Rotation 0 is mandatory */
        gud_property_req_t rotation = {.prop = GUD_PROPERTY_ROTATION, .val = GUD_ROTATION_0};
        answer(&rotation, sizeof(rotation));
        return;
    }
    case GUD_REQ_GET_CONNECTORS: {
        gud_connector_descriptor_req_t connector = {
            .connector_type = GUD_CONNECTOR_TYPE_PANEL,
            /* The host polls the status every 10 seconds, see GUD_REQ_GET_CONNECTOR_STATUS */
            .flags = GUD_CONNECTOR_FLAGS_POLL_STATUS,
        };
        answer(&connector, sizeof(connector));
        return;
    }
    default:
        break;
    }

    /* Connector requests carry the connector index in wValue, there is one connector */
    if (wValue != 0) {
        last_status = GUD_STATUS_INVALID_PARAMETER;
        return;
    }
    switch (bRequest) {
    case GUD_REQ_GET_CONNECTOR_PROPERTIES:
        /* None, an empty answer */
        answer(NULL, 0);
        return;
    case GUD_REQ_GET_CONNECTOR_STATUS: {
        /*
         * A desktop like GNOME only picks up the connector of a hotplugged display when it
         * changes, and it misses the change while the display is being set up. Reporting the
         * connector as connected a while after attaching gives it a change it sees.
         */
        uint8_t status = (now_ms() - attached_ms >= CONNECTOR_CONNECT_DELAY_MS)
                             ? GUD_CONNECTOR_STATUS_CONNECTED
                             : GUD_CONNECTOR_STATUS_DISCONNECTED;
        answer(&status, sizeof(status));
        return;
    }
    case GUD_REQ_GET_CONNECTOR_MODES: {
        /* No blanking, the clock gives the refresh rate */
        gud_display_mode_req_t mode = {
            .clock = DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_REFRESH_HZ / 1000,
            .hdisplay = DISPLAY_WIDTH,
            .hsync_start = DISPLAY_WIDTH,
            .hsync_end = DISPLAY_WIDTH,
            .htotal = DISPLAY_WIDTH,
            .vdisplay = DISPLAY_HEIGHT,
            .vsync_start = DISPLAY_HEIGHT,
            .vsync_end = DISPLAY_HEIGHT,
            .vtotal = DISPLAY_HEIGHT,
            .flags = GUD_DISPLAY_MODE_FLAG_PREFERRED,
        };
        answer(&mode, sizeof(mode));
        return;
    }
    default:
        /* EDID and TV modes are not supported, the request is stalled */
        last_status = GUD_STATUS_REQUEST_NOT_SUPPORTED;
        return;
    }
}

static uint8_t set_buffer(const uint8_t *data, size_t data_length) {
    gud_set_buffer_req_t req;
    if (data_length != sizeof(req)) {
        return GUD_STATUS_PROTOCOL_ERROR;
    }
    memcpy(&req, data, sizeof(req));

    bool inside = (req.width > 0) && (req.height > 0) && (req.x < DISPLAY_WIDTH) &&
                  (req.y < DISPLAY_HEIGHT) && (req.width <= DISPLAY_WIDTH - req.x) &&
                  (req.height <= DISPLAY_HEIGHT - req.y);
    if (!inside || (current_format == 0) ||
        (req.length != line_length(current_format, req.width) * req.height)) {
        return GUD_STATUS_INVALID_PARAMETER;
    }

    uint32_t transfer_length = req.length;
    if (req.compression != 0) {
        /* Only compression types the device announced */
        if ((req.compression & ~DISPLAY_COMPRESSION) != 0) {
            return GUD_STATUS_INVALID_PARAMETER;
        }
        transfer_length = req.compressed_length;
    }
    if ((transfer_length == 0) || (transfer_length > DISPLAY_MAX_BUFFER_SIZE)) {
        return GUD_STATUS_INVALID_PARAMETER;
    }

    pending.active = true;
    pending.req = req;
    pending.transfer_length = transfer_length;
    pending.received = 0;
    return GUD_STATUS_OK;
}

static uint8_t set_state_check(const uint8_t *data, size_t data_length) {
    gud_state_req_t state;
    if (data_length < sizeof(state) ||
        ((data_length - sizeof(state)) % sizeof(gud_property_req_t)) != 0) {
        return GUD_STATUS_PROTOCOL_ERROR;
    }
    memcpy(&state, data, sizeof(state));

    if ((state.mode.hdisplay != DISPLAY_WIDTH) || (state.mode.vdisplay != DISPLAY_HEIGHT) ||
        !is_offered_format(state.format) || (state.connector != 0)) {
        return GUD_STATUS_INVALID_PARAMETER;
    }
    for (size_t offset = sizeof(state); offset < data_length;
         offset += sizeof(gud_property_req_t)) {
        gud_property_req_t property;
        memcpy(&property, &data[offset], sizeof(property));
        if ((property.prop == GUD_PROPERTY_ROTATION) && (property.val != GUD_ROTATION_0)) {
            return GUD_STATUS_INVALID_PARAMETER;
        }
    }

    checked_state = state;
    state_checked = true;
    return GUD_STATUS_OK;
}

static uint8_t set_state_commit(void) {
    if (!state_checked) {
        return GUD_STATUS_PROTOCOL_ERROR;
    }
    if (checked_state.format != current_format) {
        printf("Display: %ux%u, format 0x%02x\n", checked_state.mode.hdisplay,
               checked_state.mode.vdisplay, checked_state.format);
    }
    current_format = checked_state.format;
    state_checked = false;
    return GUD_STATUS_OK;
}

static uint8_t set_enable(const char *name, bool *enabled, const uint8_t *data,
                          size_t data_length) {
    if (data_length != 1) {
        return GUD_STATUS_PROTOCOL_ERROR;
    }
    if (*enabled != (data[0] != 0)) {
        *enabled = (data[0] != 0);
        printf("Display: %s %s\n", name, *enabled ? "enabled" : "disabled");
    }
    return GUD_STATUS_OK;
}

static void set_request(uint8_t bRequest, const uint8_t *data, size_t data_length) {
    switch (bRequest) {
    case GUD_REQ_SET_CONNECTOR_FORCE_DETECT:
        last_status = GUD_STATUS_OK;
        break;
    case GUD_REQ_SET_BUFFER:
        last_status = set_buffer(data, data_length);
        break;
    case GUD_REQ_SET_STATE_CHECK:
        last_status = set_state_check(data, data_length);
        break;
    case GUD_REQ_SET_STATE_COMMIT:
        last_status = set_state_commit();
        break;
    case GUD_REQ_SET_CONTROLLER_ENABLE:
        last_status = set_enable("controller", &controller_enabled, data, data_length);
        break;
    case GUD_REQ_SET_DISPLAY_ENABLE:
        last_status = set_enable("output", &display_enabled, data, data_length);
        break;
    default:
        last_status = GUD_STATUS_REQUEST_NOT_SUPPORTED;
        break;
    }
}

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    uint8_t bRequestType = setup[0];
    uint8_t bRequest = setup[1];
    uint16_t wValue = (setup[3] << 8) | setup[2];
    uint16_t wIndex = (setup[5] << 8) | setup[4];

    if ((bRequestType != REQUEST_TYPE_VENDOR_INTERFACE_IN) &&
        (bRequestType != REQUEST_TYPE_VENDOR_INTERFACE_OUT)) {
        // Standard requests like SET_CONFIGURATION need nothing from the display
        return;
    }
    if (wIndex != GUD_INTERFACE) {
        last_status = GUD_STATUS_INVALID_PARAMETER;
    } else if (bRequestType == REQUEST_TYPE_VENDOR_INTERFACE_IN) {
        get_request(bRequest, wValue);
    } else {
        set_request(bRequest, data, data_length);
    }
}

static void store_rectangle(const gud_set_buffer_req_t *rect, const uint8_t *pixels) {
    uint32_t bits = format_bits_per_pixel(current_format);
    if ((bits % 8) != 0) {
        // Formats below 8 bits per pixel are not kept in the framebuffer yet
        return;
    }
    uint32_t bytes_per_pixel = bits / 8;
    uint32_t pitch = DISPLAY_WIDTH * bytes_per_pixel;
    uint32_t rect_pitch = rect->width * bytes_per_pixel;
    for (uint32_t line = 0; line < rect->height; line++) {
        memcpy(&framebuffer[(rect->y + line) * pitch + rect->x * bytes_per_pixel],
               &pixels[line * rect_pitch], rect_pitch);
    }
    framebuffer_changed = true;
}

static void endpoint_data(uint8_t ep, const uint8_t *data, size_t data_length) {
    (void)ep;
    if (!pending.active || (data_length > pending.transfer_length - pending.received)) {
        // Pixel data without a matching GUD_REQ_SET_BUFFER, drop it
        stats_errors++;
        pending.active = false;
        return;
    }
    memcpy(&transfer_buffer[pending.received], data, data_length);
    pending.received += data_length;
    if (pending.received < pending.transfer_length) {
        return;
    }

    pending.active = false;
    if (pending.req.compression != 0) {
        // Decompress transfer_buffer here once compression is announced
        stats_errors++;
        return;
    }
    store_rectangle(&pending.req, transfer_buffer);
    stats_updates++;
    stats_bytes += pending.transfer_length;
    stats_last_rect = pending.req;
}

/* Widen a colour channel of the given number of bits to 8 bits */
static uint32_t widen(uint32_t value, uint32_t bits) {
    return (value * 255) / ((1u << bits) - 1);
}

/* The window shows XRGB8888, framebuffer formats are little endian like DRM formats */
static void framebuffer_to_xrgb8888(const uint8_t *fb, uint8_t format, uint32_t *pixels) {
    for (size_t idx = 0; idx < DISPLAY_WIDTH * DISPLAY_HEIGHT; idx++) {
        uint32_t r = 0, g = 0, b = 0;
        switch (format) {
        case GUD_PIXEL_FORMAT_R8:
            r = g = b = fb[idx];
            break;
        case GUD_PIXEL_FORMAT_RGB332:
            r = widen(fb[idx] >> 5, 3);
            g = widen((fb[idx] >> 2) & 0x07, 3);
            b = widen(fb[idx] & 0x03, 2);
            break;
        case GUD_PIXEL_FORMAT_RGB565: {
            uint32_t pixel = fb[2 * idx] | (fb[2 * idx + 1] << 8);
            r = widen(pixel >> 11, 5);
            g = widen((pixel >> 5) & 0x3f, 6);
            b = widen(pixel & 0x1f, 5);
            break;
        }
        case GUD_PIXEL_FORMAT_RGB888:
            b = fb[3 * idx];
            g = fb[3 * idx + 1];
            r = fb[3 * idx + 2];
            break;
        case GUD_PIXEL_FORMAT_XRGB8888:
        case GUD_PIXEL_FORMAT_ARGB8888:
            b = fb[4 * idx];
            g = fb[4 * idx + 1];
            r = fb[4 * idx + 2];
            break;
        default:
            // Formats below 8 bits per pixel are not kept in the framebuffer yet
            break;
        }
        pixels[idx] = (r << 16) | (g << 8) | b;
    }
}

static void reset(void) {
    attached_ms = now_ms();
    last_status = GUD_STATUS_OK;
    state_checked = false;
    current_format = 0;
    controller_enabled = false;
    display_enabled = false;
    pending.active = false;
    memset(framebuffer, 0, sizeof(framebuffer));
    framebuffer_changed = true;
}

static const usbip_device_t gud_display_device = {
    .device_desc = usb_device_descriptor,
    .config_desc = usb_gud_configuration,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .ep_data_callback = endpoint_data,
    .reset_callback = reset,
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    static uint8_t frame[sizeof(framebuffer)];
    static uint32_t pixels[DISPLAY_WIDTH * DISPLAY_HEIGHT];

    if (!usbip_server_start(&gud_display_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB GUD Display Device Activated\n");

    // Without a desktop the device runs without a window
    bool window_open = gud_display_window_open(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    bool shown_on = false;
    uint64_t next_stats_ms = now_ms() + 1000;

    while (!usbip_server_interrupted()) {
        if (window_open) {
            if (!gud_display_window_handle_events(WINDOW_FRAME_INTERVAL_MS)) {
                // Closing the window stops the device
                break;
            }
        } else {
            usleep(WINDOW_FRAME_INTERVAL_MS * 1000);
        }

        bool print_stats = (now_ms() >= next_stats_ms);
        usbip_device_critical_section_enter();
        bool changed = framebuffer_changed;
        bool display_on = controller_enabled && display_enabled;
        uint8_t format = current_format;
        if (changed) {
            memcpy(frame, framebuffer, sizeof(frame));
            framebuffer_changed = false;
        }
        uint32_t updates = stats_updates;
        uint32_t bytes = stats_bytes;
        uint32_t errors = stats_errors;
        gud_set_buffer_req_t rect = stats_last_rect;
        if (print_stats) {
            stats_updates = 0;
            stats_bytes = 0;
            stats_errors = 0;
        }
        usbip_device_critical_section_exit();

        if (window_open && (changed || (display_on != shown_on))) {
            framebuffer_to_xrgb8888(frame, format, pixels);
            gud_display_window_show(pixels, display_on);
            shown_on = display_on;
        }

        if (print_stats) {
            next_stats_ms += 1000;
            if ((updates > 0) || (errors > 0)) {
                printf("Display: %u updates, %u bytes, last %ux%u at %u,%u, %u errors\n", updates,
                       bytes, rect.width, rect.height, rect.x, rect.y, errors);
            }
        }
    }

    if (window_open) {
        gud_display_window_close();
    }
    usbip_server_stop();

    return EXIT_SUCCESS;
}
