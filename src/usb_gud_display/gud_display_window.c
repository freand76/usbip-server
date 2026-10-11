/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2026

 SPDX-License-Identifier: MIT
 See the LICENSE file in the repository.

 code repository located at:
        https://github.com/freand76/usbip-server
********************************************************/

#include <stdio.h>

#include <SDL3/SDL.h>

#include "gud_display_window.h"

/* The display is small, show it twice as big with crisp pixels */
#define WINDOW_SCALE (2)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static int display_width = 0;

bool gud_display_window_open(int width, int height) {
    /* Ctrl-C is handled by the server, not by SDL */
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Window: no display window, %s\n", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("Virtual GUD Display", width * WINDOW_SCALE, height * WINDOW_SCALE,
                              SDL_WINDOW_RESIZABLE);
    renderer = (window != NULL) ? SDL_CreateRenderer(window, NULL) : NULL;
    texture = (renderer != NULL) ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING, width, height)
                                 : NULL;
    if (texture == NULL) {
        fprintf(stderr, "Window: no display window, %s\n", SDL_GetError());
        gud_display_window_close();
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    /* Keep the aspect ratio when the window is resized */
    SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    display_width = width;

    gud_display_window_show(NULL, false);
    return true;
}

void gud_display_window_show(const uint32_t *pixels, bool display_on) {
    if (renderer == NULL) {
        return;
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (display_on && (pixels != NULL)) {
        SDL_UpdateTexture(texture, NULL, pixels, display_width * (int)sizeof(uint32_t));
        SDL_RenderTexture(renderer, texture, NULL, NULL);
    }
    SDL_RenderPresent(renderer);
}

bool gud_display_window_handle_events(int timeout_ms) {
    SDL_Event event;
    bool open = true;
    /* Wait for the first event at most timeout_ms, then take what is queued */
    if (SDL_WaitEventTimeout(&event, timeout_ms)) {
        do {
            if (event.type == SDL_EVENT_QUIT) {
                open = false;
            }
        } while (SDL_PollEvent(&event));
    }
    return open;
}

void gud_display_window_close(void) {
    if (texture != NULL) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}
