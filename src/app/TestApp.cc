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

using namespace Verbose;

static volatile int keepRunning = 3;

void intHandler(int) {
    if (keepRunning == 3) {
	INFO("Ctrl-C received, Do it 3 times if you reaaly want to quit");
    }

    if (keepRunning > 0) {
	keepRunning--;
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, intHandler);

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

    UsbEndpoint ep1(0x81, 2, 64, 10);
    UsbEndpoint* epList0[1] = { &ep1 };
    UsbInterface iface0(0, 0, 0xff, 1, 2, 0, 1, epList0);

    UsbEndpoint ep2(0x82, 2, 64, 10);
    UsbEndpoint* epList1[1] = { &ep2 };
    UsbInterface iface1(1, 0, 0xff, 3, 4, 0, 1, epList1);
    UsbInterface* interfaceList[2] = { &iface0, &iface1 };
    UsbConfiguration config1(1, 0, 0xc0, 100, 2, interfaceList);
    UsbConfiguration* configurationList[1] = { &config1 };

    unsigned char buffer[512];
    memset(buffer, 0, 512);
    int size = config1.GenerateDescriptor(buffer, 0);
    INFO_VECTOR("ConfigurationDescriptor", buffer, size);

    UsbDevice dev(0x00fa, 0xc001, 0x1234, 0xff, 0, 0, 1, configurationList);

    test.AddDevice(&dev, "My First Virtual Device", "1-1", 2, 3, USB_SPEED_HIGH);
    INFO("Starting server");
    if (!test.StartServer()) {
	ERROR("Could not start server");
	return EXIT_FAILURE;
    }

    while(keepRunning > 0) {
	usleep(500*1000);
    }

    INFO("Stopping server");
    test.StopServer();

    return EXIT_SUCCESS;
}
