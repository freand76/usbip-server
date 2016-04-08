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
		     UsbConfiguration** configurationArray,
		     UsbString* usbString,
		     uint8_t iManufacturer,
		     uint8_t iProduct,
		     uint8_t iSerialNumber) {
    this->deviceConnected = false;
    this->idVendor = idVendor;
    this->idProduct = idProduct;
    this->bcdDevice = bcdDevice;
    this->bDeviceClass = bDeviceClass;
    this->bDeviceSubClass = bDeviceSubClass;
    this->bDeviceProtocol = bDeviceProtocol;
    this->bNumConfigurations = bNumConfigurations;
    this->configurationArray = configurationArray;
    this->usbString = usbString;
    this->iManufacturer = iManufacturer;
    this->iProduct = iProduct;
    this->iSerialNumber = iSerialNumber;
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

    DEBUG("Get Endpoint %.2x", endpoint);
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
    int pos = 0;

    switch(bDescriptorType) {
    case 0x01:
	pos += SetUint(18,                  dataOut, pos, 1);
	pos += SetUint(1,                   dataOut, pos, 1);
	pos += SetUint(USB_VERSION,         dataOut, pos, 2);
	pos += SetUint(bDeviceClass,        dataOut, pos, 1);
	pos += SetUint(bDeviceSubClass,     dataOut, pos, 1);
	pos += SetUint(bDeviceProtocol,     dataOut, pos, 1);
	if (transferLength < 64) {
	    pos += SetUint(transferLength,  dataOut, pos, 1);
	} else {
	    pos += SetUint(64,              dataOut, pos, 1);
	}
	pos += SetUint(idVendor,            dataOut, pos, 2);
	pos += SetUint(idProduct,           dataOut, pos, 2);
	pos += SetUint(bcdDevice,           dataOut, pos, 2);
	pos += SetUint(0,                   dataOut, pos, 1);
	pos += SetUint(0,                   dataOut, pos, 1);
	pos += SetUint(0,                   dataOut, pos, 1);
	pos += SetUint(bNumConfigurations,  dataOut, pos, 1);
	return pos;
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
	pos += SetUint(10,                  dataOut, pos, 1);
	pos += SetUint(6,                   dataOut, pos, 1);
	pos += SetUint(USB_VERSION,         dataOut, pos, 2);
	pos += SetUint(bDeviceClass,        dataOut, pos, 1);
	pos += SetUint(bDeviceSubClass,     dataOut, pos, 1);
	pos += SetUint(bDeviceProtocol,     dataOut, pos, 1);
	pos += SetUint(64,                  dataOut, pos, 1);
	pos += SetUint(0,                   dataOut, pos, 1);
	pos += SetUint(0,                   dataOut, pos, 1);
	return pos;
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
