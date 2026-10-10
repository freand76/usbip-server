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

#ifndef GUD_DISPLAY_WINDOW_H_
#define GUD_DISPLAY_WINDOW_H_

#include <stdbool.h>
#include <stdint.h>

/* Open a window showing a display of width x height pixels, false if there is no desktop */
bool gud_display_window_open(int width, int height);

/* Show a frame of XRGB8888 pixels, or a dark screen while the display is off */
void gud_display_window_show(const uint32_t *pixels, bool display_on);

/* Handle window events for at most timeout_ms, false once the window was closed */
bool gud_display_window_handle_events(int timeout_ms);

void gud_display_window_close(void);

#endif
