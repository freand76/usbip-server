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

#ifndef APP_BASE_H
#define APP_BASE_H

#include "Verbose.h"
#include "UsbIpServer.h"

using namespace Verbose;

class AppBase {
public:
    bool HandleArguments(int argc, char* argv[]) {
        if (findArgument("-h", argc, argv) || findArgument("--help", argc, argv)) {
            printf("Usage:\n");
            printf("-h / --help     - Help text\n");
            printf("-v              - Verbose [INFO]\n");
            printf("-vv             - Verbose [DEBUG]\n");
            printf("\n");
            return false;
        }

        if (findArgument("-v", argc, argv)) {
            SetVerboseLevel(LEVEL_INFO);
        }

        if (findArgument("-vv", argc, argv)) {
            SetVerboseLevel(LEVEL_DEBUG);
        }

        INFO("Command: %s", argv[0]);
        for (int idx = 1; idx < argc; idx++) {
            DEBUG("  arg%d = %s", idx, argv[idx]);
        }

        return true;
    }

    void AddDevice(UsbDevice* dev, string path, string busId, int busNum, int devNum, enum usb_device_speed speed) {
        usbIpServer.AddDevice(dev, path, busId, busNum, devNum, speed);
    }

    bool Init() {
        if (usbIpServer.Init()) {
            INFO("Init OK");
            return true;
        }
        return false;
    }

    bool Start() {
        printf("Starting server\n");
        if (!usbIpServer.StartServer()) {
            ERROR("Could not start server");
            return false;
        }
        return true;
    }

    void Stop() {
        printf("Stopping server:\n");
        if (usbIpServer.ConnectedClients()) {
            printf("\n");
            printf(" - Press Ctrl-C a couple of times if you do not want\n");
            printf("   to wait for all clients to disconnect\n\n");
        }
        usbIpServer.StopServer();
    }

private:
    static bool findArgument(const char* arg, int argc, char* argv[]) {
        for (int idx = 0; idx < argc; idx++) {
            if (strcmp(arg, argv[idx]) == 0) {
                return true;
            }
        }
        return false;
    }

    UsbIpServer usbIpServer;

};

#endif // APP_BASE_H
