#ifndef USB_IP_DRIVER_H
#define USB_IP_DRIVER_H

#include <linux/usb/ch9.h>

#include <wx/wx.h>
#include <wx/sckipc.h>
#include "UsbDevice.h"

class UsbDeviceItem {
public:
    UsbDeviceItem(UsbDevice* d = NULL,
                  wxString path = "",
                  wxString busId = "",
                  int busNum = 0,
                  int devNum = 0,
                  enum usb_device_speed speed = USB_SPEED_HIGH) {
        this->d = d;
        this->path = path;
        this->busId = busId;
        this->busNum = busNum;
        this->devNum = devNum;
        this->speed = speed;    
    }
    int GetPath(unsigned char* buffer, int offset) {
        return CopyString(path, buffer, offset, 256);
    };
    int GetBusId(unsigned char* buffer, int offset) {
        return CopyString(busId, buffer, offset, 32);
    };
    UsbDevice* d;
    int busNum;
    int devNum;
    int speed;
private:
    int CopyString(wxString str, unsigned char* buffer, int offset, int maxLen) {
        memset(&buffer[offset], 0, maxLen);
        wxCharBuffer chBuf = str.mb_str();
        memcpy(&buffer[offset], chBuf.data(), str.length());
        return maxLen;
    }
    wxString path;
    wxString busId;
};

class UsbIpServer {
public:
    UsbIpServer(int tcpPort = 3240);
    ~UsbIpServer();
    void AddDevice(UsbDevice* dev, wxString path, wxString busId, int busNum, int devNum, enum usb_device_speed speed);
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

    UsbDeviceItem usbDeviceItem;
    
};

#endif // USB_IP_DRIVER_H
