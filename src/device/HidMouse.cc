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

#include <signal.h>
#include <string.h>
#include "HidMouse.h"
#include "UsbUtil.h"
#include "AppBase.h"

using namespace UsbUtil;

/* Debug */
#include "Verbose.h"
using namespace Verbose;

#define MOUSE_REPORT_ID 100

int HidMouseInterface::GenerateConfigurationDescriptor(uint8_t* buffer, int offset) {
    int pos = offset;
    pos += UsbInterface::GenerateConfigurationDescriptor(buffer, pos);
    int rSize = GenerateHIDReportDescriptor(NULL, 0);

    pos += SetUint(9,      buffer, pos, 1);
    pos += SetUint(0x21,   buffer, pos, 1);
    pos += SetUint(0x110,  buffer, pos, 2);
    pos += SetUint(0,      buffer, pos, 1);
    pos += SetUint(1,      buffer, pos, 1);
    pos += SetUint(0x22,   buffer, pos, 1);
    pos += SetUint(rSize,  buffer, pos, 2);

    return pos - offset;
}

static uint8_t reportDescriptorHID[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, MOUSE_REPORT_ID,         //     REPORT_ID
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0
};

int HidMouseInterface::GenerateHIDReportDescriptor(uint8_t* buffer, int offset) {
    int pos = offset;
    for (unsigned int idx = 0; idx < sizeof(reportDescriptorHID); idx++) {
	pos += SetUint(reportDescriptorHID[idx], buffer, pos, 1);
    }
    return pos-offset;
}

int HidMouseInterface::GetDescriptor(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bDescriptorType = setup[3];

    if (bDescriptorType == 0x22) {
	return GenerateHIDReportDescriptor(replyBuffer, 0);
    }

    return UsbInterface::GetDescriptor(setup, data, replyBuffer, bufLength);
}

int HidMouseEndpoint::Data(uint8_t* inData, uint8_t* outBuffer, int length) {
    (void)inData;
    (void)length;

    outBuffer[0] = MOUSE_REPORT_ID;
    outBuffer[1] = event.but;
    outBuffer[2] = event.x;
    outBuffer[3] = event.y;

    event.but = 0;
    event.x = 0;
    event.y = 0;
    event.free = true;

    return 4;
}

bool HidMouse::Move(int but, int x, int y) {
    if (endpoint.event.free) {
	endpoint.event.but = but;
	endpoint.event.x = x;
	endpoint.event.y = y;
	endpoint.event.free = false;
	return true;
    }
    return false;
}

static volatile int keepRunning = 3;

void intHandler(int) {
    static int panicCounter = 0;
    if (keepRunning == 3) {
	fprintf(stderr, "\nCtrl-C received, Do it 3 times if you really want to quit\n");
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

int main(int argc, char* argv[]) {
    AppBase app;
    if (!app.HandleArguments(argc, argv)) {
	exit(0);
    }
    
    signal(SIGINT, intHandler);

    if (!app.Init()) {
	return EXIT_FAILURE;
    }
    
    HidMouse mouse(0x00fa, 0xc001, 0x1234);
    app.AddDevice(&mouse, "My First Virtual Mouse", "1-1", 2, 3, USB_SPEED_FULL);

    if (!app.Start()) {
	return EXIT_FAILURE;
    }
    
    while(keepRunning > 0) {
	usleep(500*1000);
	if (mouse.IsConnected()) {
	    if (!mouse.Move(0, 20, 20)) {
		INFO("- Mouse event-queue overflow\n");
	    } else {
		INFO("- Mouse move");
	    }
	}
    }

    app.Stop();
    
    return EXIT_SUCCESS;
}
