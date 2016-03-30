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

#include <wx/wx.h>
#include <wx/sckipc.h>

#include <cstdio>
#include <thread>

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
    this->serverSocket = NULL;
    this->serverWorkerActive = false;
    this->killServerWorker = false;
    this->activeClients = 0;
}

UsbIpServer::~UsbIpServer() {
    if (serverSocket != NULL) {
	delete serverSocket;
    }
}

void UsbIpServer::AddDevice(UsbDevice* dev, string path, string busId, int busNum, int devNum, enum usb_device_speed speed) {
    usbIpDevice = UsbIpDevice(dev, path, busId, busNum, devNum, speed);
}

bool UsbIpServer::Init() {
    wxIPV4address addr;
    if(!addr.Service(tcpPort)) {
	ERROR("TCP port");
	return false;
    }

    serverSocket = new wxSocketServer(addr);
    if (serverSocket == NULL) {
	ERROR("ServerSocket");
	return false;
    }

    wxSocketFlags flags = serverSocket->GetFlags();
    flags |= wxSOCKET_REUSEADDR;
    serverSocket->SetFlags(flags);

    if (!serverSocket->Ok()) {
	ERROR("ServerSocket not ok");
	serverSocket->Destroy();
	serverSocket = NULL;
	return false;
    }

    return true;
}

bool UsbIpServer::StartServer() {
    std::thread* server = new std::thread(&UsbIpServer::ServerWorker, this);
    if (server == NULL) {
	return false;
    }

    while(!serverWorkerActive) {
	milliSleep(100);
    }

    return true;
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
	    milliSleep(1000);
	}
    } else {
	ERROR("Cannot stop inactive server...");
    }
}

void UsbIpServer::ServerWorker() {
    serverWorkerActive = true;
    while (!killServerWorker) {
	wxSocketBase * clientSocket = serverSocket->Accept(false);
	if (clientSocket == NULL) {
	    milliSleep(500);
	    continue;
	}

	INFO("Incoming socket connection");
	if (!StartConnectionThread(clientSocket)) {
	    clientSocket->Destroy();
	}
    }
    serverWorkerActive = false;
}

bool UsbIpServer::StartConnectionThread(wxSocketBase* clientSocket) {
    std::thread* connection = new std::thread(&UsbIpServer::ConnectionWorker, this, clientSocket);
    if (connection == NULL) {
	return false;
    }
    return true;
}

#define MESSAGE_BUFFER_SIZE 512
void UsbIpServer::ConnectionWorker(wxSocketBase* clientSocket) {
    unsigned char buffer[MESSAGE_BUFFER_SIZE];
    activeClients++;

    while(clientSocket->IsConnected()) {
	clientSocket->Read(buffer, MESSAGE_BUFFER_SIZE);
	int readBytes = clientSocket->LastReadCount();

	INFO("Got %d bytes from socket", readBytes);
	if (readBytes > 0) {
	    UsbIpProtocolHandler(clientSocket, buffer, readBytes);
	}
    }
    clientSocket->Destroy();
    activeClients--;
}

void UsbIpServer::UsbIpProtocolHandler(wxSocketBase* clientSocket, unsigned char* buffer, int len) {
    if (len < 8) {
	return;
    }

    unsigned int cmd = GetUint(buffer, 0, 4);
    switch(cmd) {
    case 0x00000001:
	/* USBIP_CMD_SUBMIT */
	UsbIpHandleURB(clientSocket, buffer, len);
	return;
    case 0x00000003:
	ERROR("Unhandled Command %.8x", cmd);
	return;
    default:
	break;
    }

    unsigned int version = GetUint(buffer, 0, 2);
    if (version != 0x111) {
	ERROR("Wrong protocol version %.4x", version);
	return;
    }
    cmd = GetUint(buffer, 2, 2);

    switch(cmd) {
    case 0x8005:
	/* OP_REQ_DEVLIST */
	UsbIpReplyDeviceList(clientSocket);
	break;
    case 0x8003:
	UspIpReplyImport(clientSocket, buffer, len);
	break;
    default:
	ERROR("Unknown command: %.4x\n", cmd);
	break;
    }

}

void UsbIpServer::UsbIpReplyDeviceList(wxSocketBase* clientSocket) {
    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    int pos = 0;
    pos += SetUint(0x0111, tx_buffer, pos, 2); /* Version */
    pos += SetUint(0x0005, tx_buffer, pos, 2); /* Reply   */
    pos += SetUint(0, tx_buffer, pos, 4);      /* Status  */
    pos += SetUint(1, tx_buffer, pos, 4);      /* N devices */

    pos += usbIpDevice.FillDeviceData(tx_buffer, pos, true);

    clientSocket->Write(tx_buffer, pos);
}

void UsbIpServer::UspIpReplyImport(wxSocketBase* clientSocket, unsigned char* buffer, int len) {
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
    clientSocket->Write(tx_buffer, pos);
}

void UsbIpServer::UsbIpHandleURB(wxSocketBase* clientSocket, unsigned char* buffer, int len) {
    INFO("URB In [%d]", len);
    int seqNum = GetUint(buffer, 4, 4);
    int devId = GetUint(buffer, 8, 4);
    int ep = GetUint(buffer, 16, 4);
    int transferFlags = GetUint(buffer, 20, 4);
    int transferBufferLength = GetUint(buffer, 24, 4);

    INFO("  URB: SeqNum %.8x DevId %.8x", seqNum, devId);
    INFO("  URB: Flags %.4x EP%d ", transferFlags, ep);
    INFO("  URB: TransferBufferLength %d", transferBufferLength);

    // int inOut = GetUint(buffer, 12, 4);
    // int startFrame = GetUint(buffer, 28, 4);
    // int nofPackets = GetUint(buffer, 32, 4);
    // int interval = GetUint(buffer, 36, 4);
    // INFO("Seq=%d : Id=%d : I/O=%d : EP=%d : Flags=%d : BufLength=%d : StartFrame=%d : NofPackets=%d : Interval=%d", seqNum, devId, inOut, ep, flags, bufLength, startFrame, nofPackets, interval);

    INFO_VECTOR("USB Setup", &buffer[40], 8);

    unsigned char usbReply[64];
    int usbdataLength = usbIpDevice.TxRx(&buffer[40], &buffer[48], usbReply, transferBufferLength);
    INFO_VECTOR("USB Reply", usbReply, usbdataLength);

    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));
    int pos = 0;
    pos += SetUint(0x00000003, tx_buffer, pos, 4); /* Reply */
    pos += SetUint(seqNum, tx_buffer, pos, 4);     /* Seq */
    pos += SetUint(0, tx_buffer, pos, 4);          /* devid */
    pos += SetUint(0, tx_buffer, pos, 4);          /* direction */
    pos += SetUint(ep, tx_buffer, pos, 4);         /* EP */
    pos += SetUint(0, tx_buffer, pos, 4);          /* status */
    pos += SetUint(usbdataLength, tx_buffer, pos, 4); /* Length */
    pos += SetUint(0, tx_buffer, pos, 4);          /* Start Frame */
    pos += SetUint(0, tx_buffer, pos, 4);          /* NumberOfPackets */
    pos += SetUint(0, tx_buffer, pos, 4);          /* Error Count */
    pos += SetUint(0, tx_buffer, pos, 8);          /* Setup */
    pos += AddData(usbReply, tx_buffer, pos, usbdataLength); /* Data */

    clientSocket->Write(tx_buffer, pos);
    INFO("URB Out [%d]", pos);
}
