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

/**
 * @file gud_display_window.h
 * @brief The window showing the display's framebuffer, the only part that uses SDL3
 *
 * All functions must be called from the same thread, the program's main thread.
 */

#ifndef GUD_DISPLAY_WINDOW_H_
#define GUD_DISPLAY_WINDOW_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Open a window showing a display of width x height pixels
 *
 * The window is twice the display size and keeps its aspect ratio when resized.
 *
 * @return false if there is no desktop, the device then runs without a window
 */
bool gud_display_window_open(int width, int height);

/**
 * @brief Show a frame of XRGB8888 pixels, or a dark screen while the display is off
 *
 * @param pixels     width x height pixels, may be NULL while the display is off
 * @param display_on false shows a dark screen
 */
void gud_display_window_show(const uint32_t *pixels, bool display_on);

/**
 * @brief Handle window events, waiting at most timeout_ms for the first one
 *
 * @return false once the window was closed
 */
bool gud_display_window_handle_events(int timeout_ms);

/** @brief Close the window */
void gud_display_window_close(void);

#endif
