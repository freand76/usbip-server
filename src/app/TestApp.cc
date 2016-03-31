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
	INFO("Ctrl-C received, Do it 3 times if you reaaly want to quit");
    }

    if (keepRunning > 0) {
	keepRunning--;
    } else {
	panicCounter++;
	if (panicCounter == 5) {
	    exit(0);
	}
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, intHandler);

    SetVerboseLevel(LEVEL_INFO);
    INFO("Command: %s", argv[0]);
    for (int idx = 1; idx < argc; idx++) {
	INFO("  arg%d = %s", idx, argv[idx]);
    }

    UsbIpServer test;
    if (test.Init()) {
	INFO("Init OK");
    } else {
	return false;
    }

    HidMouse mouse(0x00fa, 0xc001, 0x1234);
    test.AddDevice(&mouse, "My First Virtual Mouse", "1-1", 2, 3, USB_SPEED_FULL);

    INFO("Starting server");
    if (!test.StartServer()) {
	ERROR("Could not start server");
	return EXIT_FAILURE;
    }

    while(keepRunning > 0) {
	usleep(500*1000);
	if (mouse.IsConnected()) {
	    if (!mouse.Move(0, 20, 20)) {
		printf(".\n");
	    }
	}
    }

    INFO("Stopping server");
    test.StopServer();

    return EXIT_SUCCESS;
}
