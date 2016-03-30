#include "UsbDevice.h"
#include "UsbInterface.h"
#include "UsbUtil.h"
#include <wx/wx.h>

using namespace UsbUtil;

UsbDevice::UsbDevice(uint16_t idVendor,
		     uint16_t idProduct,
		     uint16_t bcdDevice,
		     uint8_t bDeviceClass,
		     uint8_t bDeviceSubClass,
		     uint8_t bDeviceProtocol,
		     uint8_t bNumConfigurations,
		     UsbConfiguration** configurationArray) {
    this->idVendor = idVendor;
    this->idProduct = idProduct;
    this->bcdDevice = bcdDevice;
    this->bDeviceClass = bDeviceClass;
    this->bDeviceSubClass = bDeviceSubClass;
    this->bDeviceProtocol = bDeviceProtocol;
    this->bNumConfigurations = bNumConfigurations;
    this->configurationArray = configurationArray;
}

int UsbDevice::TxRx(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength) {
    int direction = (setup[0] & 0x80) >> 7;
 
    if (direction == 1) {
	wxPrintf("Out\n");
	return OutRequest(setup, data, replyBuffer, bufLength);
    } else {
	wxPrintf("In\n");
	return InRequest(setup, data, replyBuffer, bufLength);
    }
}

int UsbDevice::OutRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength) {
    //int type = (setup[0] & 0x60) >> 5;
    int recipient = setup[0] & 0x05;
    
    if (recipient == 0) {
	wxPrintf("Device\n");
	return DeviceRequest(setup, data, replyBuffer, bufLength);
    } else {
	wxPrintf("Out Recipient %d\n", recipient); 
    }
    return 0;
}

int UsbDevice::InRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength) {
    (void)setup;
    (void)data;
    (void)replyBuffer;
    (void)bufLength;

    int bRequest = setup[1];
    int reqIndex =  setup[2] | (setup[3] << 8);

    switch(bRequest) {
    case 0x09:
	wxPrintf("Set Configuration: %d\n", reqIndex);
	break;
    }
    
    return 0;
}

int UsbDevice::DeviceRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength) {
    (void)data;

    int packetSize = 0;
    int bRequest = setup[1];
    int reqIndex =  (setup[2] << 8) | setup[3];
    int pos = 0;
    
    switch(bRequest) {
    case 0x06:
	switch(reqIndex) {
	case 0x0001:
	    replyBuffer[0] = 18;
	    replyBuffer[1] = 1;
	    SetUint(0x200, replyBuffer, 2, 2);
	    replyBuffer[4] = bDeviceClass;
	    replyBuffer[5] = bDeviceSubClass;
	    replyBuffer[6] = bDeviceProtocol;
	    if (bufLength < 64) {
		replyBuffer[7] = bufLength;
	    } else {
		replyBuffer[7] = 64;
	    }
	    SetUint(idVendor, replyBuffer, 8, 2);
	    SetUint(idProduct, replyBuffer, 10, 2);
	    SetUint(bcdDevice, replyBuffer, 12, 2);
	    replyBuffer[14] = 0;
	    replyBuffer[15] = 0;
	    replyBuffer[16] = 0;
	    replyBuffer[17] = bNumConfigurations; 
	    packetSize = 18;
	    break;
	case 0x0002:
	    pos = 0;
	    for (int idx = 0; idx < bNumConfigurations; idx++) {
		pos += configurationArray[idx]->GenerateDescriptor(replyBuffer, pos);
	    }
	    wxPrintf("ConfigSize = %d\n", pos);
	    packetSize = 25;
	    break;
	default:
	    break;
	}
    default:
	break;
    }

    if (packetSize > bufLength) {
	packetSize = bufLength;
	wxPrintf("Trunc package: %d\n", packetSize); 
    }
    return packetSize;
}
