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

#ifndef USB_IP_DRIVER_H
#define USB_IP_DRIVER_H

#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

#include "UsbDevice.h"
#include "UsbIpDevice.h"

using namespace std;

class UsbIpServer {
public:
    UsbIpServer(int tcpPort = 3240);
    ~UsbIpServer();
    void AddDevice(UsbDevice* dev, string path, string busId, int busNum, int devNum, enum usb_device_speed speed);
    bool Init();
    bool StartServer();
    void StopServer();
    void ServerWorker();
    bool StartConnectionThread(int clientSocketFd);
    void ConnectionWorker(int clientSocketFd);
private:
    void UsbIpProtocolHandler(int clientSocketFd, unsigned char* buffer, int len);
    void UsbIpReplyDeviceList(int clientSocketFd);
    void UsbIpReplyImport(int clientSocketFd, unsigned char* buffer, int len);
    void UsbIpUnlinkURB(int clientSocketFd, unsigned char* buffer, int len);
    void UsbIpHandleURB(int clientSocketFd, unsigned char* buffer, int len);

    std::thread* serverThread;
    std::vector<std::thread*> connectionThreads;

    int serverSocketFd;
    bool serverWorkerActive;
    bool killServerWorker;
    int activeClients;
    int tcpPort;

    UsbIpDevice usbIpDevice;

};

#endif // USB_IP_DRIVER_H
