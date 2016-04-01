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

int UsbDevice::TxRx(uint8_t endpoint, uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    if (endpoint == 0) {
	/* Control */
	uint8_t bmRequestType = usbSetup[0];
	int bfRecipient = bmRequestType & 0x05;

	switch(bfRecipient) {
	case 0:
	    return DeviceRequest(usbSetup, dataIn, dataOut, transferLength);
	case 1:
	    return InterfaceRequest(usbSetup, dataIn, dataOut, transferLength);
	default:
	    DEBUG("Unknown bfRecipient %d", bfRecipient);
	}
	return EP_STALL;
    }

    if (dataOut == NULL) {
	return transferLength;
    }

    return configurationArray[0]->GetEndpoint(endpoint)->Data(dataIn, dataOut, transferLength);
}

void UsbDevice::Disconnect() {
    deviceConnected = false;
}

int UsbDevice::DeviceRequest(uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    uint8_t bmRequestType = usbSetup[0];
    int bfDataDirection = bmRequestType & 0x80;
    if (bfDataDirection) {
	/* OutRequest */
	return OutRequest(usbSetup, dataIn, dataOut, transferLength);
    } else {
	/* InRequest */
	return InRequest(usbSetup, dataIn, dataOut, transferLength);
    }
}

int UsbDevice::OutRequest(uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    uint8_t bRequest = usbSetup[1];
    DEBUG("UsbDevice: OutRequest %.2x", bRequest);

    int packetSize = 0;

    if (bRequest == 0x00) {
	SetUint(0x0001, dataOut, 0, 2);
	packetSize = 2;
    } else if (bRequest == 0x06) {
	packetSize =  GetDescriptor(usbSetup, dataIn, dataOut, transferLength);
    } else {
	ERROR("Unknown bRequest: %.2x", bRequest);
    }

    return packetSize;
}

int UsbDevice::GetDescriptor(uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    (void)dataIn;
    uint8_t bDescriptorType = usbSetup[3];
    uint8_t bDescriptorIndex = usbSetup[2];

    switch(bDescriptorType) {
    case 0x01:
	if (dataOut != NULL) {
	    dataOut[0] = 18;
	    dataOut[1] = 1;
	    SetUint(USB_VERSION, dataOut, 2, 2);
	    dataOut[4] = bDeviceClass;
	    dataOut[5] = bDeviceSubClass;
	    dataOut[6] = bDeviceProtocol;
	    if (transferLength < 64) {
		dataOut[7] = transferLength;
	    } else {
		dataOut[7] = 64;
	    }
	    SetUint(idVendor, dataOut, 8, 2);
	    SetUint(idProduct, dataOut, 10, 2);
	    SetUint(bcdDevice, dataOut, 12, 2);
	    dataOut[14] = 0;
	    dataOut[15] = 0;
	    dataOut[16] = 0;
	    dataOut[17] = bNumConfigurations;
	}
	return 18;
    case 0x02:
	/* ConfigurationDescriptor */
	// FIXME Handle EP size better!!!
	// [ 5787.496570] usb 8-1: Using ep0 maxpacket: 8
	// [ 5787.497786] usb 8-1: config 1 interface 0 altsetting 0 bulk endpoint 0x81 has invalid maxpacket 64
	// [ 5787.497792] usb 8-1: config 1 interface 1 altsetting 0 bulk endpoint 0x82 has invalid maxpacket 64
	if (bDescriptorIndex < bNumConfigurations) {
	    return configurationArray[bDescriptorIndex]->GenerateConfigurationDescriptor(dataOut, 0);
	}
	return EP_STALL;
    case 0x06:
	if (dataOut != NULL) {
	    dataOut[0] = 10;
	    dataOut[1] = 6;
	    SetUint(USB_VERSION, dataOut, 2, 2);
	    dataOut[4] = bDeviceClass;
	    dataOut[5] = bDeviceSubClass;
	    dataOut[6] = bDeviceProtocol;
	    dataOut[7] = 64;
	    dataOut[8] = 0;
	    dataOut[9] = 0;
	}
	return 10;
    default:
	ERROR("Unknown bDescriptorType:bDescriptorIndexbRequest %.2x:%.2x", bDescriptorType, bDescriptorIndex);
	break;
    }
    return EP_STALL;
}

int UsbDevice::InRequest(uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    (void)dataIn;
    (void)dataOut;
    (void)transferLength;

    uint8_t bRequest = usbSetup[1];
    DEBUG("UsbDevice: InRequest %.2x", bRequest);
    switch(bRequest) {
    case 0x09:
    {
	/* Set Configuration */
	int configrationValue = GetUint(usbSetup, 2, 2);
	DEBUG("UsbDevice: Set Configuration: %d", configrationValue);
	DEBUG("Device Connected");
	deviceConnected = true;
	return 0;
    }
    }

    return EP_STALL;
}

int UsbDevice::InterfaceRequest(uint8_t* usbSetup, uint8_t* dataIn, uint8_t* dataOut, int transferLength) {
    uint8_t bInterfaceIndex = usbSetup[2];
    DEBUG("UsbDevice: InterfaceRequest %.2x", bInterfaceIndex);
    if (bInterfaceIndex < configurationArray[0]->bNumInterfaces) {
	return configurationArray[0]->interfaceArray[bInterfaceIndex]->InterfaceRequest(usbSetup, dataIn, dataOut, transferLength);
    }
    return EP_STALL;
}
