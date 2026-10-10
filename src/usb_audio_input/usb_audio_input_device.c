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

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "usbip_server_api.h"
#include "usb_audio_input_descriptors.h"

#define TWO_PI (6.28318530718f)

// One alien pulse per second: a warbling chirp sweeping three octaves up while it fades out
#define PULSE_PERIOD_SAMPLES (AUDIO_SAMPLE_RATE)
#define PULSE_LENGTH_SAMPLES (AUDIO_SAMPLE_RATE * 4 / 10)
#define PULSE_ATTACK_SAMPLES (AUDIO_SAMPLE_RATE / 100)
#define SWEEP_START_HZ (220.0f)
#define SWEEP_END_HZ (1760.0f)
#define WARBLE_HZ (30.0f)
#define WARBLE_DEPTH (0.15f)
#define PEAK_AMPLITUDE (0.5f * 32767.0f)

// Only used from device callbacks, which never run at the same time
static uint8_t streaming_alternate = 0;
static uint32_t sample_index = 0;
static float phase = 0.0f;

static int16_t next_sample(void) {
    float value = 0.0f;

    if (sample_index < PULSE_LENGTH_SAMPLES) {
        float progress = (float)sample_index / PULSE_LENGTH_SAMPLES;
        float time_s = (float)sample_index / AUDIO_SAMPLE_RATE;

        // Exponential sweep, so every octave takes the same time, with a fast warble on top
        float frequency = SWEEP_START_HZ * powf(SWEEP_END_HZ / SWEEP_START_HZ, progress);
        frequency *= 1.0f + WARBLE_DEPTH * sinf(TWO_PI * WARBLE_HZ * time_s);
        phase += TWO_PI * frequency / AUDIO_SAMPLE_RATE;
        if (phase > TWO_PI) {
            phase -= TWO_PI;
        }

        // Short attack, then a fade that reaches silence at the end of the pulse
        float envelope = 1.0f - progress;
        if (sample_index < PULSE_ATTACK_SAMPLES) {
            envelope *= (float)sample_index / PULSE_ATTACK_SAMPLES;
        }
        value = envelope * sinf(phase);
    }

    sample_index = (sample_index + 1) % PULSE_PERIOD_SAMPLES;
    return (int16_t)(value * PEAK_AMPLITUDE);
}

static size_t iso_in_data(uint8_t ep, uint8_t *buffer, size_t max_length) {
    (void)ep;
    size_t samples = AUDIO_SAMPLES_PER_PACKET;
    if (samples * AUDIO_BYTES_PER_SAMPLE > max_length) {
        samples = max_length / AUDIO_BYTES_PER_SAMPLE;
    }

    // One packet per millisecond, 16 bit little endian samples
    for (size_t idx = 0; idx < samples; idx++) {
        uint16_t sample = (uint16_t)next_sample();
        buffer[2 * idx + 0] = sample & 0xff;
        buffer[2 * idx + 1] = sample >> 8;
    }
    return samples * AUDIO_BYTES_PER_SAMPLE;
}

static void control_data(const uint8_t *setup, const uint8_t *data, size_t data_length) {
    (void)data;
    (void)data_length;
    uint8_t bRequestType = setup[0];
    uint8_t bRequest = setup[1];
    uint8_t alternate = setup[2];
    uint8_t interface = setup[4];

    if ((bRequestType == 0x01) && (bRequest == USB_REQUEST_SET_INTERFACE)) {
        // The host selects alternate setting 1 to record and 0 to stop
        if ((interface == AUDIO_STREAMING_INTERFACE) && (alternate != streaming_alternate)) {
            streaming_alternate = alternate;
            sample_index = 0;
            phase = 0.0f;
            printf("Recording %s\n", (streaming_alternate != 0) ? "started" : "stopped");
        }
    } else if ((bRequestType == 0x81) && (bRequest == USB_REQUEST_GET_INTERFACE)) {
        uint8_t current = (interface == AUDIO_STREAMING_INTERFACE) ? streaming_alternate : 0;
        usbip_device_transmit(0, &current, sizeof(current));
    }
}

static void reset(void) {
    streaming_alternate = 0;
}

static const usbip_device_t audio_device = {
    .device_desc = usb_device_descriptor,
    .config_desc = usb_audio_configuration,
    .langid_desc = langid_desc,
    .usb_strings = usb_strings,
    .control_data_callback = control_data,
    .reset_callback = reset,
    .iso_in_callback = iso_in_data,
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!usbip_server_start(&audio_device)) {
        printf("ERROR: usbip_server_start failed\n");
        return EXIT_FAILURE;
    }

    printf("USB Audio Input Device Activated\n");

    while (!usbip_server_interrupted()) {
        usleep(100000);
    }

    usbip_server_stop();

    return EXIT_SUCCESS;
}
