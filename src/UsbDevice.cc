#include "UsbDevice.h"
#include <wx/wx.h>

static int USB_SetUint(unsigned int value, unsigned char* buffer, int offset, int byteWidth) {
    int val = value;
    for (int idx = 0; idx < byteWidth; idx++) {
	buffer[offset + idx] = val & 0xff;
	val = val >> 8;
    }
    return byteWidth;
};

UsbDevice::UsbDevice(uint16_t idVendor,
		     uint16_t idProduct,
		     uint16_t bcdDevice,
		     uint8_t bDeviceClass,
		     uint8_t bDeviceSubClass,
		     uint8_t bDeviceProtocol) {
    this->idVendor = idVendor;
    this->idProduct = idProduct;
    this->bcdDevice = bcdDevice;
    this->bDeviceClass = bDeviceClass;
    this->bDeviceSubClass = bDeviceSubClass;
    this->bDeviceProtocol = bDeviceProtocol;
    this->bConfigurationValue = 1;
    this->bNumConfigurations = 1;
    this->bNumInterfaces = 1;
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
    int reqIndex =  (setup[2] << 8) | setup[3];

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
    
    switch(bRequest) {
    case 0x06:
	switch(reqIndex) {
	case 0x0001:
	    replyBuffer[0] = 18;
	    replyBuffer[1] = 1;
	    USB_SetUint(0x200, replyBuffer, 2, 2);
	    replyBuffer[4] = bDeviceClass;
	    replyBuffer[5] = bDeviceSubClass;
	    replyBuffer[6] = bDeviceProtocol;
	    if (bufLength < 64) {
		replyBuffer[7] = bufLength;
	    } else {
		replyBuffer[7] = 64;
	    }
	    USB_SetUint(idVendor, replyBuffer, 8, 2);
	    USB_SetUint(idProduct, replyBuffer, 10, 2);
	    USB_SetUint(bcdDevice, replyBuffer, 12, 2);
	    replyBuffer[14] = 0;
	    replyBuffer[15] = 0;
	    replyBuffer[16] = 0;
	    replyBuffer[17] = bNumConfigurations; 
	    packetSize = 18;
	    break;
	case 0x0002:
	    replyBuffer[0] = 9;
	    replyBuffer[1] = 2;
	    replyBuffer[2] = 25;
	    replyBuffer[4] = 1;
	    replyBuffer[5] = 1;
	    replyBuffer[6] = 0;
	    replyBuffer[7] = 0xc0;
	    replyBuffer[8] = 100;
	    
	    replyBuffer[9] = 9;
	    replyBuffer[10] = 4;
	    replyBuffer[11] = 0;
	    replyBuffer[12] = 0;
	    replyBuffer[13] = 1;
	    replyBuffer[14] = 0xff;
	    replyBuffer[15] = 0;
	    replyBuffer[16] = 0;
	    replyBuffer[17] = 0;
	    
	    replyBuffer[18] = 7;
	    replyBuffer[19] = 5;
	    replyBuffer[20] = 0x81;
	    replyBuffer[21] = 2;
	    replyBuffer[22] = 64;
	    replyBuffer[23] = 0;
	    replyBuffer[24] = 10;
	    
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
