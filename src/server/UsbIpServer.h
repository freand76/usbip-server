#ifndef USB_IP_DRIVER_H
#define USB_IP_DRIVER_H

#include <wx/wx.h>
#include <wx/sckipc.h>

#include <string>

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
    bool StartConnectionThread(wxSocketBase* clientSocket);
    void ConnectionWorker(wxSocketBase* clientSocket);
private:
    void UsbIpProtocolHandler(wxSocketBase* clientSocket, unsigned char* buffer, int len);
    void UsbIpReplyDeviceList(wxSocketBase* clientSocket);
    void UspIpReplyImport(wxSocketBase* clientSocket, unsigned char* buffer, int len);
    void UsbIpHandleURB(wxSocketBase* clientSocket, unsigned char* buffer, int len);
    wxSocketServer* serverSocket;
    bool serverWorkerActive;
    bool killServerWorker;
    int activeClients;
    int tcpPort;

    UsbIpDevice usbIpDevice;
    
};

#endif // USB_IP_DRIVER_H
