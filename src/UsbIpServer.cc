#include <wx/wx.h>
#include <wx/sckipc.h>
#include <wx/thread.h>

#include "UsbIpServer.h"
#include "NetworkUtil.h"

using namespace NetworkUtil;

class ServerThread : public wxThread {
public:
    ServerThread(UsbIpServer* usbIpServer)
        : wxThread(wxTHREAD_DETACHED)
        { this->usbIpServer = usbIpServer; }
protected:
    virtual ExitCode Entry() {
	usbIpServer->ServerWorker();
	return NULL;
    }
    UsbIpServer* usbIpServer;
};

class ConnectionThread : public wxThread {
public:
    ConnectionThread(UsbIpServer* usbIpServer, wxSocketBase* clientSocket)
        : wxThread(wxTHREAD_DETACHED) {
	this->usbIpServer = usbIpServer;
	this->clientSocket = clientSocket;
	}
protected:
    virtual ExitCode Entry() {
	usbIpServer->ConnectionWorker(clientSocket);
	return NULL;
    }
    UsbIpServer* usbIpServer;
    wxSocketBase* clientSocket;
};

UsbIpServer::UsbIpServer(int tcpPort) {
    this->tcpPort = tcpPort;
    this->serverSocket = NULL;
    this->serverWorkerActive = false;
    this->killServerWorker = false;
    this->activeClients = 0;
}

UsbIpServer::~UsbIpServer() {
    if (serverSocket != NULL) {
	wxPrintf("Delete serversocket\n");
	delete serverSocket;
    }
}

void UsbIpServer::AddDevice(UsbDevice* dev, wxString path, wxString busId, int busNum, int devNum, int speed) {
    usbDeviceItem = UsbDeviceItem(dev, path, busId, busNum, devNum, speed);
}

bool UsbIpServer::Init() {
    wxIPV4address addr;
    if(!addr.Service(tcpPort)) {
	wxPrintf("No address\n");
	return false;
    }

    serverSocket = new wxSocketServer(addr);
    if (serverSocket == NULL) {
	wxPrintf("No serversocket\n");
	return false;
    }

    wxSocketFlags flags = serverSocket->GetFlags();
    flags |= wxSOCKET_REUSEADDR;
    serverSocket->SetFlags(flags);
    
    if (!serverSocket->Ok()) {
	wxPrintf("Not OK\n");
	serverSocket->Destroy();
        serverSocket = NULL;
	return false;
    }

    return true;
}

bool UsbIpServer::StartServer() {
    wxThread* thread = new ServerThread(this);    
    if ( thread->Run() != wxTHREAD_NO_ERROR ) {
        delete thread;
        return false;
    }
    
    while(!serverWorkerActive) {
	wxMilliSleep(100);
    }

    return true;
}

void UsbIpServer::StopServer() {
    if (serverWorkerActive) {
	killServerWorker = true;
	while((serverWorkerActive) || (activeClients > 0)) {
	    wxPrintf("%d\n", activeClients);
	    wxSleep(1);
	}
    } else {
	wxPrintf("Server not active...\n");
    }
}

void UsbIpServer::ServerWorker() {
    serverWorkerActive = true;
    while (!killServerWorker) {
	wxSocketBase * clientSocket = serverSocket->Accept(false);
        if (clientSocket == NULL) {
	    wxMilliSleep(500);
	    continue;
	}

	wxPrintf("Connect\n");
	if (!StartConnectionThread(clientSocket)) {
	    clientSocket->Destroy();
	}
    }
    serverWorkerActive = false;
}

bool UsbIpServer::StartConnectionThread(wxSocketBase* clientSocket) {
    wxThread* thread = new ConnectionThread(this, clientSocket);    
    if ( thread->Run() != wxTHREAD_NO_ERROR ) {
	delete thread;
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

	wxPrintf("Read %d byes\n",  readBytes);
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
    }
	
    unsigned int version = GetUint(buffer, 0, 2);
    wxPrintf("Version %.4x\n", version);
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
	wxPrintf("Unknown command: %.4x\n", cmd);
	break;
    }

}

static int FillDeviceData(UsbDeviceItem* item, unsigned char* buffer, int offset) {
    int pos = offset;

    pos += item->GetPath(buffer, pos);
    pos += item->GetBusId(buffer, pos);
    pos += SetUint(item->busNum, buffer, pos, 4); /* */
    pos += SetUint(item->devNum, buffer, pos, 4); /* */
    pos += SetUint(item->speed, buffer, pos, 4); /* */
    
    pos += SetUint(item->d->idVendor, buffer, pos, 2); /* */
    pos += SetUint(item->d->idProduct, buffer, pos, 2); /* */
    pos += SetUint(item->d->bcdDevice, buffer, pos, 2); /* */
    pos += SetUint(item->d->bDeviceClass, buffer, pos, 1); /* */
    pos += SetUint(item->d->bDeviceSubClass, buffer, pos, 1); /* */
    pos += SetUint(item->d->bDeviceProtocol, buffer, pos, 1); /* */
    pos += SetUint(item->d->bConfigurationValue, buffer, pos, 1); /* */
    pos += SetUint(item->d->bNumConfigurations, buffer, pos, 1); /* */
    pos += SetUint(item->d->configurationArray[0]->bNumInterfaces, buffer, pos, 1); /* */

    return pos - offset;
}

void UsbIpServer::UsbIpReplyDeviceList(wxSocketBase* clientSocket) {
    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    int pos = 0;
    pos += SetUint(0x0111, tx_buffer, pos, 2); /* Version */
    pos += SetUint(0x0005, tx_buffer, pos, 2); /* Reply   */
    pos += SetUint(0, tx_buffer, pos, 4);      /* Status  */
    pos += SetUint(1, tx_buffer, pos, 4);      /* N devices */

    pos += FillDeviceData(&usbDeviceItem, tx_buffer, pos);

    /* Interface data */
    pos += SetUint(0x03, tx_buffer, pos, 1); /* Interfaceclass = HID */
    pos += SetUint(0, tx_buffer, pos, 1);    /* */
    pos += SetUint(0, tx_buffer, pos, 1); /* */
    pos += SetUint(0, tx_buffer, pos, 1); /* */

    clientSocket->Write(tx_buffer, pos);
}

void UsbIpServer::UspIpReplyImport(wxSocketBase* clientSocket, unsigned char* buffer, int len) {
    unsigned char tx_buffer[512];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    if (len != 40) {
	return;
    }
    wxString busId = wxString::Format("%s", (char*)&buffer[8]);

    int pos = 0;
    pos += SetUint(0x0111, tx_buffer, pos, 2); /* Version */
    pos += SetUint(0x0003, tx_buffer, pos, 2); /* Reply   */
    pos += SetUint(0, tx_buffer, pos, 4);      /* Status  */
    
    pos += FillDeviceData(&usbDeviceItem, tx_buffer, pos);
    clientSocket->Write(tx_buffer, pos);
    wxPrintf("TX: %d\n", pos);
}

void UsbIpServer::UsbIpHandleURB(wxSocketBase* clientSocket, unsigned char* buffer, int len) {
    wxPrintf("URB: %d\n", len);
    int seqNum = GetUint(buffer, 4, 4);
    int devId = GetUint(buffer, 8, 4);
    int inOut = GetUint(buffer, 12, 4);
    int ep = GetUint(buffer, 16, 4);
    int flags = GetUint(buffer, 20, 4);
    int bufLength = GetUint(buffer, 24, 4);
    int startFrame = GetUint(buffer, 28, 4);
    int nofPackets = GetUint(buffer, 32, 4);
    int interval = GetUint(buffer, 36, 4);

    wxPrintf("Seq=%d : Id=%d : I/O=%d : EP=%d : Flags=%d : BufLength=%d : StartFrame=%d : NofPackets=%d : Interval=%d\n", seqNum, devId, inOut, ep, flags, bufLength, startFrame, nofPackets, interval);

    wxPrintf("Setup: ");
    for (int idx = 0; idx < 8; idx++) {
	wxPrintf("%.2x, ", buffer[40+idx]);
    }
    wxPrintf("\n");

    unsigned char usbReply[64];
    int usbdataLength = usbDeviceItem.d->TxRx(&buffer[40], &buffer[40], usbReply, bufLength); 
    wxPrintf("Reply Length %d\n", usbdataLength);
    
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
    wxPrintf("URB TX: %d\n", pos);
}
