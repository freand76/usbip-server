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
#include "BulkIO.h"
#include "UsbUtil.h"
#include "AppBase.h"
#include "Verbose.h"

using namespace UsbUtil;
using namespace Verbose;

int BulkInputEndpoint::Data(uint8_t* dataIn, uint8_t* dataOut, int length) {
    (void)dataIn;
    sprintf((char*)dataOut, "APA %d", length);
    return strlen((char*)dataOut);
}

int BulkOutputEndpoint::Data(uint8_t* dataIn, uint8_t* dataOut, int length) {
    (void)dataOut;
    printf("%d:%s\n", length, dataIn);
    return 0;
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
    
    BulkIO bulkIO(0x00fa, 0xc001, 0x1234);
    app.AddDevice(&bulkIO, "My First Virtual BulkIO", "1-1", 2, 3, USB_SPEED_FULL);

    if (!app.Start()) {
	return EXIT_FAILURE;
    }
    
    while(keepRunning > 0) {
	usleep(500*1000);
	if (bulkIO.IsConnected()) {
	    INFO("- Connected");
	}
    }

    app.Stop();
    
    return EXIT_SUCCESS;
}
