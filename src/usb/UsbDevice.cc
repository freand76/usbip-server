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

#include "UsbDevice.h"
#include "UsbInterface.h"
#include "UsbUtil.h"
#include "Verbose.h"

#define USB_VERSION 0x0110

using namespace Verbose;
using namespace UsbUtil;

UsbDevice::UsbDevice(uint16_t idVendor,
		     uint16_t idProduct,
		     uint16_t bcdDevice,
		     uint8_t bDeviceClass,
		     uint8_t bDeviceSubClass,
		     uint8_t bDeviceProtocol,
		     uint8_t bNumConfigurations,
		     UsbConfiguration** configurationArray) {
    this->deviceConnected = false;
    this->idVendor = idVendor;
    this->idProduct = idProduct;
    this->bcdDevice = bcdDevice;
    this->bDeviceClass = bDeviceClass;
    this->bDeviceSubClass = bDeviceSubClass;
    this->bDeviceProtocol = bDeviceProtocol;
    this->bNumConfigurations = bNumConfigurations;
    this->configurationArray = configurationArray;
}

int UsbDevice::TxRx(uint8_t endpoint, uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    if (endpoint == 0) {
	/* Control */
	uint8_t bmRequestType = setup[0];
	int bfRecipient = bmRequestType & 0x05;

	switch(bfRecipient) {
	case 0:
	    return DeviceRequest(setup, data, replyBuffer, bufLength);
	case 1:
	    return InterfaceRequest(setup, data, replyBuffer, bufLength);
	default:
	    INFO("Unknown bfRecipient %d", bfRecipient);
	}
	return EP_STALL;
    }

    return configurationArray[0]->GetEndpoint(endpoint)->Data(data, replyBuffer, bufLength);
}

void UsbDevice::Disconnect() {
    deviceConnected = false;
}

int UsbDevice::DeviceRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bmRequestType = setup[0];
    int bfDataDirection = bmRequestType & 0x80;
    if (bfDataDirection) {
	/* OutRequest */
	return OutRequest(setup, data, replyBuffer, bufLength);
    } else {
	/* InRequest */
	return InRequest(setup, data, replyBuffer, bufLength);
    }
}

int UsbDevice::OutRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bRequest = setup[1];
    INFO("UsbDevice: OutRequest %.2x", bRequest);

    int packetSize = 0;

    if (bRequest == 0x00) {
	SetUint(0x0001, replyBuffer, 0, 2);
	packetSize = 2;
    } else if (bRequest == 0x06) {
	packetSize =  GetDescriptor(setup, data, replyBuffer, bufLength);
    } else {
	ERROR("Unknown bRequest: %.2x", bRequest);
    }

    if (packetSize > bufLength) {
	packetSize = bufLength;
	INFO("Trunc package: %d", packetSize);
    }
    return packetSize;
}

int UsbDevice::GetDescriptor(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    (void)data;
    uint8_t bDescriptorType = setup[3];
    uint8_t bDescriptorIndex = setup[2];

    switch(bDescriptorType) {
    case 0x01:
	replyBuffer[0] = 18;
	replyBuffer[1] = 1;
	SetUint(USB_VERSION, replyBuffer, 2, 2);
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
	return 18;
    case 0x02:
	/* ConfigurationDescriptor */
	// FIXME Handle EP size better!!!
	// [ 5787.496570] usb 8-1: Using ep0 maxpacket: 8
	// [ 5787.497786] usb 8-1: config 1 interface 0 altsetting 0 bulk endpoint 0x81 has invalid maxpacket 64
	// [ 5787.497792] usb 8-1: config 1 interface 1 altsetting 0 bulk endpoint 0x82 has invalid maxpacket 64
	if (bDescriptorIndex < bNumConfigurations) {
	    return configurationArray[bDescriptorIndex]->GenerateConfigurationDescriptor(replyBuffer, 0);
	}
	return EP_STALL;
    case 0x06:
	replyBuffer[0] = 10;
	replyBuffer[1] = 6;
	SetUint(USB_VERSION, replyBuffer, 2, 2);
	replyBuffer[4] = bDeviceClass;
	replyBuffer[5] = bDeviceSubClass;
	replyBuffer[6] = bDeviceProtocol;
	replyBuffer[7] = 64;
	replyBuffer[8] = 0;
	replyBuffer[9] = 0;
	return 10;
    default:
	ERROR("Unknown bDescriptorType:bDescriptorIndexbRequest %.2x:%.2x", bDescriptorType, bDescriptorIndex);
	break;
    }
    return EP_STALL;
}

int UsbDevice::InRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    (void)data;
    (void)replyBuffer;
    (void)bufLength;

    uint8_t bRequest = setup[1];
    INFO("UsbDevice: InRequest %.2x", bRequest);
    switch(bRequest) {
    case 0x09:
    {
	/* Set Configuration */
	int configrationValue = GetUint(setup, 2, 2);
	INFO("UsbDevice: Set Configuration: %d", configrationValue);
	INFO("Device Connected");
	deviceConnected = true;
	return 0;
    }
    }

    return EP_STALL;
}

int UsbDevice::InterfaceRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bInterfaceIndex = setup[2];
    INFO("UsbDevice: InterfaceRequest %.2x", bInterfaceIndex);
    if (bInterfaceIndex < configurationArray[0]->bNumInterfaces) {
	return configurationArray[0]->interfaceArray[bInterfaceIndex]->InterfaceRequest(setup, data, replyBuffer, bufLength);
    }
    return EP_STALL;
}
