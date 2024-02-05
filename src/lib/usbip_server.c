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

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usbip_device.h"
#include "usbip_protocol.h"
#include "usbip_server.h"
#include "tcpip_server.h"

#define USBIP_PORT (3240)

static const usbip_device_t *usbip_device = NULL;

static volatile int keep_running = 3;
static volatile bool worker_stopped = true;

static void int_handler(int unused) {
    (void)unused;
    static int panic_counter = 0;

    if (keep_running == 3) {
        fprintf(stderr, "\nServer: Ctrl-C received, Do it 3 times if you really want to quit\n");
    } else {
        fprintf(stderr, "\n");
    }
    fflush(stderr);

    if (keep_running > 0) {
        keep_running--;
    } else {
        panic_counter++;
        if (panic_counter == 5) {
            exit(0);
        }
    }
}

static void *usbip_server_worker_thread(void *arg) {
    (void)arg;
    worker_stopped = false;
    while (keep_running > 0) {
        if (!tcpip_server_wait_for_client()) {
            continue;
        }
        bool disconnect = false;
        printf("USBIPServer: Connected\n");
        while ((keep_running > 0) && (!disconnect)) {
            /* Process packets */
            if (usbip_protocol_is_device_imported()) {
                disconnect = usbip_protocol_process_imported_device(usbip_device);
            } else {
                disconnect = usbip_protocol_process_cmd(usbip_device);
            }
        }
        printf("USBIPServer: Close Client\n");
        tcpip_server_close_client();
    }
    printf("USBIPServer: Close Server\n");
    tcpip_server_close();
    worker_stopped = true;
    return NULL;
}

bool usbip_server_start(const usbip_device_t *usb_device) {
    /* Make it possible to kill the server */
    signal(SIGINT, int_handler);

    usbip_device = usb_device;

    bool status = tcpip_server_open(USBIP_PORT);
    pthread_t thread_id;

    if (status) {
        int err = pthread_create(&thread_id, NULL, &usbip_server_worker_thread, NULL);
        status = (err == 0);
    }
    if (status) {
        printf("USBIPServer: Started\n\n");
    }
    return status;
}

void usbip_server_stop(void) {
    keep_running = 0;
    tcpip_server_stop();
    printf("\nUSBIPServer: Stopping...\n");
    while (!worker_stopped) {
        printf("USBIPServer: Waiting for server to stop...\n");
        usleep(500000);
    }
    printf("USBIPServer: Stopped\n");
}

bool usbip_server_interrupted(void) {
    return (keep_running == 0);
}
