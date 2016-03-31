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

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>

#include <cstdio>

#include "Verbose.h"
#include "UsbIpServer.h"
#include "NetworkUtil.h"

using namespace Verbose;
using namespace NetworkUtil;

static void milliSleep(int ms) {
    usleep(ms*1000);
}

UsbIpServer::UsbIpServer(int tcpPort) {
    this->tcpPort = tcpPort;
    this->serverSocketFd = -1;
    this->serverWorkerActive = false;
    this->killServerWorker = false;
    this->activeClients = 0;
    this->serverThread = NULL;
}

UsbIpServer::~UsbIpServer() {
    if (serverSocketFd >= 0) {
	close(serverSocketFd);
    }

    for (unsigned int idx = 0; idx < connectionThreads.size(); idx++) {
	connectionThreads[idx]->join();
	delete connectionThreads[idx];
    }
}

void UsbIpServer::AddDevice(UsbDevice* dev, string path, string busId, int busNum, int devNum, enum usb_device_speed speed) {
    usbIpDevice = UsbIpDevice(dev, path, busId, busNum, devNum, speed);
}

bool UsbIpServer::Init() {
    struct sockaddr_in serverAddress;

    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0) {
	ERROR("Could not open server socket");
	return false;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(tcpPort);

    if (bind(serverSocketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
	close(serverSocketFd);
	serverSocketFd = -1;
	ERROR("Could not bind server socket");
	return false;
    }


    listen(serverSocketFd, 16);

    int flags = fcntl(serverSocketFd, F_GETFL, 0);
    if (flags < 0) {
	close(serverSocketFd);
	serverSocketFd = -1;
	ERROR("Could not get socket flags");
	return false;
    }
    flags |= O_NONBLOCK;
    if (fcntl(serverSocketFd, F_SETFL, flags) != 0) {
	close(serverSocketFd);
	serverSocketFd = -1;
	ERROR("Could not set socket flags");
	return false;
    }

    return true;
}

bool UsbIpServer::StartServer() {
    serverThread = new std::thread(&UsbIpServer::ServerWorker, this);
    if (serverThread == NULL) {
	return false;
    }

    while(!serverWorkerActive) {
	milliSleep(100);
    }

    return true;
}

bool UsbIpServer::ConnectedClients() {
    return (activeClients > 0);
}

void UsbIpServer::StopServer() {
    if (serverWorkerActive) {
	killServerWorker = true;
	int oldActioveClients = -1;
	while((serverWorkerActive) || (activeClients > 0)) {
	    if (activeClients != oldActioveClients) {
		INFO("Active socket clients %d", activeClients);
		oldActioveClients = activeClients;
	    }
	    milliSleep(500);
	}
	serverThread->join();
	delete serverThread;
	serverThread = NULL;
    } else {
	ERROR("Cannot stop inactive server...");
    }
}

void UsbIpServer::ServerWorker() {
    serverWorkerActive = true;
    while (!killServerWorker) {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);
	int clientSocketFd = accept(serverSocketFd,
				    (struct sockaddr*)&clientAddress,
				    &clientAddressLength);

	if (clientSocketFd < 0) {
	    milliSleep(500);
	    continue;
	}

	INFO("Incoming socket connection");
	if (!StartConnectionThread(clientSocketFd)) {
	    close(clientSocketFd);
	}
    }
    serverWorkerActive = false;
}

bool UsbIpServer::StartConnectionThread(int clientSocketFd) {
    std::thread* connectionThread = new std::thread(&UsbIpServer::ConnectionWorker, this, clientSocketFd);
    if (connectionThread == NULL) {
	return false;
    }

    connectionThreads.push_back(connectionThread);
    return true;
}

#define MESSAGE_BUFFER_SIZE 512
void UsbIpServer::ConnectionWorker(int clientSocketFd) {
    unsigned char buffer[MESSAGE_BUFFER_SIZE];
    activeClients++;
    INFO("Client connect");
    while(1) {
	int readBytes = read(clientSocketFd, buffer, sizeof(buffer));

	DEBUG("Got %d bytes from socket", readBytes);
	if (readBytes > 0) {
	    /* FIXME Handle multiple packets */
	    UsbIpProtocolHandler(clientSocketFd, buffer, readBytes);
	} else {
	    break;
	}
    }

    usbIpDevice.Disconnect();
    INFO("Device Disconnected");

    close(clientSocketFd);
    activeClients--;
}

void UsbIpServer::UsbIpProtocolHandler(int clientSocketFd, unsigned char* buffer, int len) {
    if (len < 8) {
	return;
    }

    unsigned int cmd = GetUint(buffer, 0, 4);
    switch(cmd) {
    case 0x00000001:
	/* USBIP_CMD_SUBMIT */
	UsbIpHandleURB(clientSocketFd, buffer, len);
	return;
    case 0x00000002:
	UsbIpUnlinkURB(clientSocketFd, buffer, len);
	return;
    default:
	break;
    }

    unsigned int version = GetUint(buffer, 0, 2);
    if (version != 0x111) {
	ERROR("Wrong protocol version %.4x", version);
	ERROR("  Unknown command %.8x", cmd);
	return;
    }
    cmd = GetUint(buffer, 2, 2);

    switch(cmd) {
    case 0x8005:
	/* OP_REQ_DEVLIST */
	UsbIpReplyDeviceList(clientSocketFd);
	break;
    case 0x8003:
	UsbIpReplyImport(clientSocketFd, buffer, len);
	break;
    default:
	ERROR("Unknown command: %.4x\n", cmd);
	break;
    }

}

void UsbIpServer::UsbIpReplyDeviceList(int clientSocketFd) {
    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    int pos = 0;
    pos += SetUint(0x0111, tx_buffer, pos, 2); /* Version */
    pos += SetUint(0x0005, tx_buffer, pos, 2); /* Reply   */
    pos += SetUint(0, tx_buffer, pos, 4);      /* Status  */
    pos += SetUint(1, tx_buffer, pos, 4);      /* N devices */

    pos += usbIpDevice.FillDeviceData(tx_buffer, pos, true);

    if (write(clientSocketFd, tx_buffer, pos) != pos) {
	ERROR("Did not write full packet to socket");
    }
}

void UsbIpServer::UsbIpReplyImport(int clientSocketFd, unsigned char* buffer, int len) {
    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    if (len != 40) {
	return;
    }
    std::string busId((char*)&buffer[8]);
    INFO("Attach Device: %s", busId.c_str());

    int pos = 0;
    pos += SetUint(0x0111, tx_buffer, pos, 2); /* Version */
    pos += SetUint(0x0003, tx_buffer, pos, 2); /* Reply   */
    pos += SetUint(0, tx_buffer, pos, 4);      /* Status  */

    pos += usbIpDevice.FillDeviceData(tx_buffer, pos, false);
    if (write(clientSocketFd, tx_buffer, pos) != pos) {
	ERROR("Did not write full packet to socket");
    }
}

void UsbIpServer::UsbIpUnlinkURB(int clientSocketFd, unsigned char* buffer, int len) {
    DEBUG("Unlink URB Out [%d]", len);
    int seqNum = GetUint(buffer, 4, 4);
    int devId = GetUint(buffer, 8, 4);
    int ep = GetUint(buffer, 16, 4);

    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));
    int pos = 0;
    pos += SetUint(0x00000004, tx_buffer, pos, 4); /* 0x00 Reply */
    pos += SetUint(seqNum, tx_buffer, pos, 4);     /* 0x04 Seq */
    pos += SetUint(devId, tx_buffer, pos, 4);      /* 0x08 devid */
    pos += SetUint(0, tx_buffer, pos, 4);          /* 0x0c direction */
    pos += SetUint(ep, tx_buffer, pos, 4);         /* 0x10 EP */
    pos += SetUint(0, tx_buffer, pos, 4);          /* 0x14 status */
    pos = 48;

    DEBUG("Unlink URB Out [%d]", pos);
    if (write(clientSocketFd, tx_buffer, pos) != pos) {
	ERROR("Did not write full packet to socket");
    }
}

void UsbIpServer::UsbIpHandleURB(int clientSocketFd, unsigned char* buffer, int len) {
    DEBUG("URB In [%d]", len);
    int seqNum = GetUint(buffer, 4, 4);
    int devId = GetUint(buffer, 8, 4);
    int ep = GetUint(buffer, 16, 4);
    int transferFlags = GetUint(buffer, 20, 4);
    int transferBufferLength = GetUint(buffer, 24, 4);
    int startFrame = GetUint(buffer, 28, 4);

    int inOut = GetUint(buffer, 12, 4);
    DEBUG("  URB: SeqNum %.8x DevId %.8x", seqNum, devId);
    DEBUG("  URB: Flags %.4x EP%d IO=%d", transferFlags, ep, inOut);
    DEBUG("  URB: TransferBufferLength %d", transferBufferLength);

    // int nofPackets = GetUint(buffer, 32, 4);
    // int interval = GetUint(buffer, 36, 4);
    // DEBUG("Seq=%d : Id=%d : I/O=%d : EP=%d : Flags=%d : BufLength=%d : StartFrame=%d : NofPackets=%d : Interval=%d", seqNum, devId, inOut, ep, flags, bufLength, startFrame, nofPackets, interval);

    DEBUG_VECTOR("USB Setup", &buffer[40], 8);

    int status = 0;
    unsigned char usbReply[64];
    int usbdataLength = usbIpDevice.TxRx(ep, &buffer[40], &buffer[48], usbReply, transferBufferLength);
    if (usbdataLength < 0) {
	status = usbdataLength;
	switch(usbdataLength) {
	case EP_STALL:
	    DEBUG("USB Stall");
	    break;
	default:
	    ERROR("USB Error %d", usbdataLength);
	    break;
	}
    } else {
	DEBUG_VECTOR("USB Reply", usbReply, usbdataLength);
    }

    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));
    int pos = 0;
    pos += SetUint(0x00000003, tx_buffer, pos, 4); /* 0x00 Reply */
    pos += SetUint(seqNum, tx_buffer, pos, 4);     /* 0x04 Seq */
    pos += SetUint(devId, tx_buffer, pos, 4);          /* 0x08 devid */
    pos += SetUint(0, tx_buffer, pos, 4);          /* 0x0c direction */
    pos += SetUint(ep, tx_buffer, pos, 4);         /* 0x10 EP */
    pos += SetUint(status, tx_buffer, pos, 4);          /* 0x14 status */
    pos += SetUint(usbdataLength, tx_buffer, pos, 4); /* 0x18 Length */
    pos += SetUint(startFrame, tx_buffer, pos, 4);          /* 0x1c Start Frame */
    pos += SetUint(0, tx_buffer, pos, 4);          /* 0x20 NumberOfPackets */
    pos += SetUint(0, tx_buffer, pos, 4);          /* 0x24 Error Count */
    pos += SetUint(0, tx_buffer, pos, 8);          /* 0x28 Setup */
    if (usbdataLength > 0) {
	pos += AddData(usbReply, tx_buffer, pos, usbdataLength); /* Data */
    }

    DEBUG("URB Out [%d]", pos);
    if (write(clientSocketFd, tx_buffer, pos) != pos) {
	ERROR("Did not write full packet to socket");
    }
}
