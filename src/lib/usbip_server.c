/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2026

 SPDX-License-Identifier: MIT
 See the LICENSE file in the repository.

 code repository located at:
        https://github.com/freand76/usbip-server
********************************************************/

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
static volatile sig_atomic_t panic_counter = 0;

/* Only async-signal-safe functions may be used from the signal handler */
static void write_stderr(const char *message) {
    if (write(STDERR_FILENO, message, strlen(message)) < 0) {
        /* Nothing to do if stderr is gone */
    }
}

static void int_handler(int unused) {
    (void)unused;

    if (keep_running == 3) {
        write_stderr("\nServer: Ctrl-C received, Do it 3 times if you really want to quit\n");
    } else {
        write_stderr("\n");
    }

    if (keep_running > 0) {
        keep_running--;
        if (keep_running == 0) {
            /* Abort any blocking client read */
            tcpip_server_stop();
        }
    } else {
        panic_counter++;
        if (panic_counter == 5) {
            _exit(0);
        }
    }
}

static void *usbip_server_worker_thread(void *arg) {
    (void)arg;
    usbip_device_mark_server_thread();
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
    struct sigaction int_action = {0};
    int_action.sa_handler = int_handler;
    sigemptyset(&int_action.sa_mask);
    int_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &int_action, NULL);

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
        /* Ctrl-C is handled by the application, never by interrupting the worker thread */
        sigset_t int_set;
        sigset_t old_set;
        sigemptyset(&int_set);
        sigaddset(&int_set, SIGINT);
        pthread_sigmask(SIG_BLOCK, &int_set, &old_set);
        int err = pthread_create(&thread_id, NULL, &usbip_server_worker_thread, NULL);
        pthread_sigmask(SIG_SETMASK, &old_set, NULL);
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
