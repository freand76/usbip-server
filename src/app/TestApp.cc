/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2016, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
	http://github.com/freand76/usbip-server
********************************************************/

#include <string.h>
#include <signal.h>

#include "Verbose.h"
#include "UsbDevice.h"
#include "UsbConfiguration.h"
#include "UsbInterface.h"
#include "UsbEndpoint.h"
#include "UsbIpServer.h"

#include "HidMouse.h"

using namespace Verbose;

static volatile int keepRunning = 3;

void intHandler(int) {
    static int panicCounter = 0;
    if (keepRunning == 3) {
	fprintf(stderr, "\nCtrl-C received, Do it 3 times if you reaaly want to quit\n");
    } else {
	fprintf(stderr, "\n");
    }
    fflush(stderr);

    if (keepRunning > 0) {
	keepRunning--;
    } else {
	panicCounter++;
	if (panicCounter == 5) {
	    exit(0);
	}
    }
}

bool findArgument(const char* arg, int argc, char* argv[]) {
    for (int idx = 0; idx < argc; idx++) {
	if (strcmp(arg, argv[idx]) == 0) {
	    return true;
	}
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (findArgument("-h", argc, argv) || findArgument("--help", argc, argv)) {
	printf("Usage:\n");
	printf("-h / --help     - Help text\n");
	printf("-v              - Verbose [INFO]\n");
	printf("-vv             - Verbose [DEBUG]\n");
	printf("\n");
	exit(0);
    }

    if (findArgument("-v", argc, argv)) {
	SetVerboseLevel(LEVEL_INFO);
    }

    if (findArgument("-vv", argc, argv)) {
	SetVerboseLevel(LEVEL_DEBUG);
    }

    signal(SIGINT, intHandler);

    INFO("Command: %s", argv[0]);
    for (int idx = 1; idx < argc; idx++) {
	DEBUG("  arg%d = %s", idx, argv[idx]);
    }

    UsbIpServer test;
    if (test.Init()) {
	INFO("Init OK");
    } else {
	return false;
    }

    HidMouse mouse(0x00fa, 0xc001, 0x1234);
    test.AddDevice(&mouse, "My First Virtual Mouse", "1-1", 2, 3, USB_SPEED_FULL);

    printf("Starting server\n");
    if (!test.StartServer()) {
	ERROR("Could not start server");
	return EXIT_FAILURE;
    }

    while(keepRunning > 0) {
	usleep(500*1000);
	if (mouse.IsConnected()) {
	    if (!mouse.Move(0, 20, 20)) {
		printf("- Mouse event-queue overflow\n");
	    }
	}
    }

    printf("Stopping server:\n");
    if (test.ConnectedClients()) {
	printf("\n");
	printf(" - Press Ctrl-C a couple of times if you do not want\n");
	printf("   to wait for all clients to disconnect\n\n");
    }
    test.StopServer();

    return EXIT_SUCCESS;
}
