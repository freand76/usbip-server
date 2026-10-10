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

/* Only the local machine can attach unless another address is given */
#define USBIP_BIND_ADDRESS_ENV "USBIP_BIND_ADDRESS"
#define USBIP_DEFAULT_BIND_ADDRESS "127.0.0.1"

static const usbip_device_t *usbip_device = NULL;
static pthread_t thread_id;

static volatile sig_atomic_t keep_running = 3;

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
        if (keep_running == 0) {
            /* Abort any blocking client read */
            tcpip_server_stop();
        }
    } else {
        panic_counter++;
        if (panic_counter == 5) {
            exit(0);
        }
    }
}

static void *usbip_server_worker_thread(void *arg) {
    (void)arg;
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
    usbip_device_stop();
    printf("USBIPServer: Close Server\n");
    tcpip_server_close();
    return NULL;
}

bool usbip_server_start(const usbip_device_t *usb_device) {
    /* Make it possible to kill the server */
    signal(SIGINT, int_handler);

    usbip_device = usb_device;

    bool status = usbip_device_init();

    const char *bind_address = getenv(USBIP_BIND_ADDRESS_ENV);
    if (bind_address == NULL) {
        bind_address = USBIP_DEFAULT_BIND_ADDRESS;
    }

    if (status) {
        status = tcpip_server_open(bind_address, USBIP_PORT);
    }

    if (status) {
        int err = pthread_create(&thread_id, NULL, &usbip_server_worker_thread, NULL);
        status = (err == 0);
    }
    if (status) {
        printf("USBIPServer: Started on %s:%d\n\n", bind_address, USBIP_PORT);
    }
    return status;
}

void usbip_server_stop(void) {
    keep_running = 0;
    tcpip_server_stop();
    printf("\nUSBIPServer: Stopping...\n");
    pthread_join(thread_id, NULL);
    printf("USBIPServer: Stopped\n");
}

bool usbip_server_interrupted(void) {
    return (keep_running == 0);
}
