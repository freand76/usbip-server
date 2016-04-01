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
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <cstdio>

#include "Verbose.h"
#include "UsbIpServer.h"
#include "NetworkUtil.h"

using namespace Verbose;
using namespace NetworkUtil;

#define USBIP_OP_HEAD_SIZE    8
#define USBIP_CMD_HEAD_SIZE   48
#define USBIP_VERSION         0x0111
#define USBIP_MAX_PACKET_SIZE 4096

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

unsigned int UsbIpServer::TcpRead(int clientSocketFd, uint8_t* buffer, unsigned int readSize) {
    unsigned int readBytes = 0;
    DEBUG("TCP Read %d", readSize);
    do {
	int rBytes = read(clientSocketFd, &buffer[readBytes], readSize - readBytes);
	DEBUG("   Read %d", rBytes);
	if (rBytes == 0) {
	    break;
	}
	readBytes += rBytes;
    } while(readBytes < readSize);

    DEBUG_VECTOR("TCP", buffer, readBytes);

    return readBytes;
}

void UsbIpServer::ConnectionWorker(int clientSocketFd) {
    uint8_t headerBuffer[USBIP_CMD_HEAD_SIZE];
    bool deviceImported = false;

    activeClients++;
    INFO("Client connect");
    while(1) {
	/* Read exactly 8 bytes */
	unsigned int headSize = USBIP_OP_HEAD_SIZE;
	if (deviceImported) {
	    headSize = USBIP_CMD_HEAD_SIZE;
	}

	unsigned int readBytes = TcpRead(clientSocketFd, headerBuffer, headSize);
	if (readBytes == 0) {
	    break;
	} else if (readBytes < headSize) {
	    ERROR("Received strange or no data over TCP/IP, closing connection");
	    break;
	}

	int txSize = 0;
	uint8_t* txBuffer = NULL;
	/* Start Protocol handler */
	if (deviceImported) {
	    txBuffer = UsbIpCommandHandler(clientSocketFd, headerBuffer, txSize);
	} else {
	    uint16_t protocolVersion = GetUint(headerBuffer, 0, 2);
	    if (protocolVersion == USBIP_VERSION) {
		/* manage command list/attach */
		unsigned int manageCmd = GetUint(headerBuffer, 2, 2);
		switch(manageCmd) {
		case 0x8005:
		    /* OP_REQ_DEVLIST */
		    txBuffer = UsbIpReplyDeviceList(txSize);
		    break;
		case 0x8003:
		    txBuffer = UsbIpReplyImport(clientSocketFd, txSize);
		    deviceImported = (txBuffer != NULL);
		    break;
		default:
		    ERROR("Unknown command: %.4x", manageCmd);
		    break;
		}
	    } else {
		ERROR("Unknown version: %.4x", protocolVersion);
	    }
	}

	/* Transmit Reply */
	if (txBuffer == NULL) {
	    ERROR("Nothing to transmit?, closing connection");
	    break;
	}

	DEBUG("TCP TxSize %d", txSize);
	DEBUG_VECTOR("TCP", txBuffer, txSize);

	/* Transmit data over TCP/IP socket */
	int wSize = write(clientSocketFd, txBuffer, txSize);
	delete[] txBuffer;

	/* OK TCP/IP transmission? */
	if (wSize != txSize) {
	    ERROR("Did not write full packet to socket, closing connection");
	    break;
	}
    }

    /* Disconnect & Close socket */
    INFO("Device disconnected");
    usbIpDevice.Disconnect();
    INFO("Client socket closed");
    close(clientSocketFd);
    activeClients--;
}

uint8_t* UsbIpServer::UsbIpReplyDeviceList(int& txSize) {
    int replySize = 12;
    replySize+= usbIpDevice.FillDeviceData(NULL, replySize, true);

    INFO("ReplySize = %d", replySize);

    uint8_t* txBuffer = new uint8_t[replySize];
    if (txBuffer != NULL) {
	memset(txBuffer, 0, replySize);

	int pos = 0;
	pos += SetUint(USBIP_VERSION, txBuffer, pos, 2); /* Version */
	pos += SetUint(0x0005, txBuffer, pos, 2); /* Reply   */
	pos += SetUint(0,      txBuffer, pos, 4); /* Status  */
	pos += SetUint(1,      txBuffer, pos, 4); /* N devices */
	pos += usbIpDevice.FillDeviceData(txBuffer, pos, true);
	txSize = replySize;
    }
    return txBuffer;
}

uint8_t* UsbIpServer::UsbIpReplyImport(int clientSocketFd, int& txSize) {
    uint8_t busIdBuffer[USBIP_BUSID_STR_LENGTH];
    if (TcpRead(clientSocketFd, busIdBuffer, sizeof(busIdBuffer)) != sizeof(busIdBuffer)) {
	return NULL;
    }

    int replySize = 8;
    replySize+= usbIpDevice.FillDeviceData(NULL, replySize, false);

    uint8_t* txBuffer = new uint8_t[replySize];
    if (txBuffer != NULL) {
	memset(txBuffer, 0, replySize);

	std::string busId((char*)busIdBuffer);
	INFO("Attach Device: %s", busId.c_str());

	int pos = 0;
	pos += SetUint(USBIP_VERSION, txBuffer, pos, 2); /* Version */
	pos += SetUint(0x0003, txBuffer, pos, 2); /* Reply   */
	pos += SetUint(0,      txBuffer, pos, 4); /* Status  */

	pos += usbIpDevice.FillDeviceData(txBuffer, pos, false);
	txSize = replySize;
    }
    return txBuffer;
}

uint8_t* UsbIpServer::UsbIpCommandHandler(int clientSocketFd, uint8_t* cmdHeadBuffer, int& txSize) {
    uint32_t command = GetUint(cmdHeadBuffer, 0, 4);
    switch(command) {
    case 0x00000001:
	/* USBIP_CMD_SUBMIT */
	return UsbIpHandleURB(clientSocketFd, cmdHeadBuffer, txSize);
    case 0x00000002:
	/* USBIP_CMD_UNLINK */
	return UsbIpUnlinkURB(cmdHeadBuffer, txSize);
    default:
	ERROR("Unkown UsbIp command %.4x", command);
	break;
    }
    return NULL;
}

uint8_t* UsbIpServer::UsbIpUnlinkURB(uint8_t* cmdHeadBuffer, int& txSize) {
    uint32_t sequenceNumber =   GetUint(cmdHeadBuffer, 0x04, 4);  /* 0x04 seqnum  */
    uint32_t devId =            GetUint(cmdHeadBuffer, 0x08, 4);  /* 0x08 devid */
    uint32_t unlinkSeqNum =     GetUint(cmdHeadBuffer, 0x14, 4);  /* 0x14 unlink seqnum */
    DEBUG("Unlink URB %.4x", unlinkSeqNum);

    uint8_t* txBuffer = new uint8_t[USBIP_CMD_HEAD_SIZE];
    if (txBuffer != NULL) {
	memset(txBuffer, 0, USBIP_CMD_HEAD_SIZE);
	SetUint(4,              txBuffer, 0x00, 4);  /* 0x00 reply  */
	SetUint(sequenceNumber, txBuffer, 0x04, 4);  /* 0x04 Seq */
	SetUint(devId,          txBuffer, 0x08, 4);  /* 0x08 devid */
	SetUint(0,              txBuffer, 0x0c, 4);  /* 0x0c direction */
	SetUint(0,              txBuffer, 0x10, 4);  /* 0x10 EP */
	SetUint(0,              txBuffer, 0x14, 4);  /* 0x14 status */
	txSize = USBIP_CMD_HEAD_SIZE;
    }
    return txBuffer;
}

uint8_t* UsbIpServer::UsbIpHandleURB(int clientSocketFd, uint8_t* cmdHeadBuffer, int& txSize) {
    uint32_t sequenceNumber =   GetUint(cmdHeadBuffer, 0x04, 4);  /* 0x04 seqnum  */
    uint32_t devId =            GetUint(cmdHeadBuffer, 0x08, 4);  /* 0x08 devid */
    uint32_t direction =        GetUint(cmdHeadBuffer, 0x0c, 4);  /* 0x0c direction */
    uint32_t ep =               GetUint(cmdHeadBuffer, 0x10, 4);  /* 0x10 ep */
    uint32_t transferFlags =    GetUint(cmdHeadBuffer, 0x14, 4);  /* 0x14 transfer_flags    */
    uint32_t transferLength =   GetUint(cmdHeadBuffer, 0x18, 4);  /* 0x18 transfer_length   */
    //uint32_t startFrame =       GetUint(cmdHeadBuffer, 0x1c, 4);  /* 0x1c start_frame       */
    uint32_t isoPackets =       GetUint(cmdHeadBuffer, 0x20, 4);  /* 0x20 number_of_packets */
    uint32_t interval =         GetUint(cmdHeadBuffer, 0x24, 4);  /* 0x24 intervale         */

    DEBUG("  URB: DevId %.8x, Flags %.4x", devId, transferFlags);
    DEBUG("  URB: EP%d, transfer_length %d", ep, transferLength);
    DEBUG("  URB: ISO Packets %d, interval %d", isoPackets, interval);
    DEBUG_VECTOR("USB Setup", &cmdHeadBuffer[0x28], 8);

    /* Dynamic create rxData in case it is needed */
    uint8_t usbSetup[8];
    memcpy(usbSetup, &cmdHeadBuffer[0x28], sizeof(usbSetup));

    uint8_t rxData[transferLength];
    /* Read incoming data if available */
    if (direction == 0) {
	if (TcpRead(clientSocketFd, rxData, transferLength) != transferLength) {
	    return NULL;
	}
    }

    uint8_t* txBuffer = new uint8_t[USBIP_CMD_HEAD_SIZE + USBIP_MAX_PACKET_SIZE];
    uint8_t* usbReply = &txBuffer[USBIP_CMD_HEAD_SIZE];
    int usbIpStatus = 0;
    unsigned int actualLength = 0;
    if (txBuffer != NULL) {
	memset(txBuffer, 0, USBIP_CMD_HEAD_SIZE + USBIP_MAX_PACKET_SIZE);
	int status = usbIpDevice.TxRx(ep, usbSetup, rxData, usbReply, transferLength);
	if (status < 0) {
	    switch(status) {
	    case EP_STALL:
		usbIpStatus = EP_STALL;
		DEBUG("USB Stall");
		break;
	    default:
		ERROR("USB Error %d", status);
		break;
	    }
	} else {
	    actualLength = status;
	}

	if (actualLength > transferLength) {
	    actualLength = transferLength;
	    DEBUG("Trunc package %d", actualLength);
	}
	DEBUG_VECTOR("USB Reply", usbReply, actualLength);

	SetUint(3,              txBuffer, 0x00, 4);  /* 0x00 reply  */
	SetUint(sequenceNumber, txBuffer, 0x04, 4);  /* 0x04 Seq */
	SetUint(devId,          txBuffer, 0x08, 4);  /* 0x08 devid */
	SetUint(0,              txBuffer, 0x0c, 4);  /* 0x0c direction */
	SetUint(ep,             txBuffer, 0x10, 4);  /* 0x10 EP */
	SetUint(usbIpStatus,    txBuffer, 0x14, 4);  /* 0x14 status */
	SetUint(actualLength,   txBuffer, 0x18, 4);  /* 0x18 actual_length */
	SetUint(0,              txBuffer, 0x1c, 4);  /* 0x1c start_frame */
	SetUint(0,              txBuffer, 0x20, 4);  /* 0x20 number_of_packets */
	SetUint(0,              txBuffer, 0x24, 4);  /* 0x24 error_count */

	if (direction == 0) {
	    txSize = USBIP_CMD_HEAD_SIZE;
	} else {
	    txSize = USBIP_CMD_HEAD_SIZE + actualLength;
	}
    }

    return txBuffer;
}
