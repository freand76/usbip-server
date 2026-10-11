/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2026

 SPDX-License-Identifier: MIT
 See the LICENSE file in the repository.

 code repository located at:
        https://github.com/freand76/usbip-server
********************************************************/

/**
 * @file usb_audio_input_descriptors.h
 * @brief Descriptors of the audio input example, out/usb_audio_input_device
 *
 * A USB Audio Class 1.0 microphone: an audio control interface, and an audio streaming
 * interface whose alternate setting 1 has an isochronous IN endpoint. See
 * docs/usb_audio_input_device.md.
 */

#ifndef USB_AUDIO_INPUT_DESCRIPTORS_H_
#define USB_AUDIO_INPUT_DESCRIPTORS_H_

#include <stdint.h>

#include "descriptor_utils.h"

//
// USB Strings
//

/** @brief The device's strings, ended by an entry with id 0 */
static const usb_string_t usb_strings[] = {
    {1, "Virtual UsbIP"},
    {2, "Virtual Audio Input"},
    {4, "ABC123"},
    {0, NULL},
};

//
// Device Descriptor
//

#define USB_VID 0xfa00
#define USB_PID 0x0004

/** @brief Device descriptor */
static const uint8_t usb_device_descriptor[USB_DEVICE_DESCRIPTOR_LENGTH] = {
    USB_DEVICE_DESCRIPTOR_LENGTH, // bLength
    USB_DESCRIPTOR_TYPE_DEVICE,   // bDescriptorType
    LOBYTE(USB_BCD_VERSION),      // bcdUSB
    HIBYTE(USB_BCD_VERSION),
    0x00,            // bDeviceClass (use class information in the interface descriptors)
    0x00,            // bDeviceSubClass
    0x00,            // bDeviceProtocol
    64,              // bMaxPacketSize
    LOBYTE(USB_VID), // idVendor
    HIBYTE(USB_VID),
    LOBYTE(USB_PID), // idProduct
    HIBYTE(USB_PID),
    LOBYTE(USB_BCD_DEVICE_REL), // bcdDevice - USB device release number
    HIBYTE(USB_BCD_DEVICE_REL),
    1, // Index of manufacturer string
    2, // Index of product string
    4, // Index of serial number string
    1, // bNumConfigurations
};

//
// Audio Format, 48 kHz 16 bit mono, one packet per millisecond
//

#define AUDIO_SAMPLE_RATE (48000)
#define AUDIO_CHANNELS (1)
#define AUDIO_BYTES_PER_SAMPLE (2)
#define AUDIO_SAMPLES_PER_PACKET (AUDIO_SAMPLE_RATE / 1000)
#define AUDIO_PACKET_SIZE (AUDIO_SAMPLES_PER_PACKET * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE)

/* High speed interval is 2^(bInterval - 1) * 125 us, 2^3 * 125 us = 1 ms */
#define AUDIO_EP_INTERVAL (4)
#define AUDIO_EP_DATA_IN (0x81)

#define AUDIO_CONTROL_INTERFACE (0)
#define AUDIO_STREAMING_INTERFACE (1)
#define AUDIO_INPUT_TERMINAL_ID (1)
#define AUDIO_OUTPUT_TERMINAL_ID (2)

//
// USB Audio Class 1.0 Definitions
//

#define USB_CLASS_AUDIO (0x01)
#define USB_SUBCLASS_AUDIOCONTROL (0x01)
#define USB_SUBCLASS_AUDIOSTREAMING (0x02)

#define USB_DESCRIPTOR_TYPE_CS_INTERFACE (0x24)
#define USB_DESCRIPTOR_TYPE_CS_ENDPOINT (0x25)

#define UAC_HEADER (0x01)
#define UAC_INPUT_TERMINAL (0x02)
#define UAC_OUTPUT_TERMINAL (0x03)
#define UAC_AS_GENERAL (0x01)
#define UAC_FORMAT_TYPE (0x02)
#define UAC_EP_GENERAL (0x01)

#define UAC_TERMINAL_USB_STREAMING (0x0101)
#define UAC_TERMINAL_MICROPHONE (0x0201)
#define UAC_FORMAT_PCM (0x0001)
#define UAC_FORMAT_TYPE_I (0x01)

//
// Configuration Descriptor
//

#define UAC_HEADER_LENGTH (9)
#define UAC_INPUT_TERMINAL_LENGTH (12)
#define UAC_OUTPUT_TERMINAL_LENGTH (9)
#define AUDIO_CONTROL_CS_LENGTH                                                                    \
    (UAC_HEADER_LENGTH + UAC_INPUT_TERMINAL_LENGTH + UAC_OUTPUT_TERMINAL_LENGTH)

#define USB_AUDIO_CONFIG_DESC_LENGTH                                                               \
    (9 +                         /* Configuration */                                               \
     9 + AUDIO_CONTROL_CS_LENGTH /* Audio control interface */                                     \
     + 9                         /* Audio streaming interface, alternate setting 0 */              \
     + 9 + 7 + 11                /* Audio streaming interface, alternate setting 1 */              \
     + 9 + 7)                    /* Isochronous endpoint */

/** @brief Configuration descriptor, followed by the descriptors of its interfaces and endpoints */
static const uint8_t usb_audio_configuration[USB_AUDIO_CONFIG_DESC_LENGTH] = {
    0x09,                                 /* bLength: Configuration Descriptor size */
    USB_DESCRIPTOR_TYPE_CONFIGURATION,    /* bDescriptorType */
    LOBYTE(USB_AUDIO_CONFIG_DESC_LENGTH), /* wTotalLength */
    HIBYTE(USB_AUDIO_CONFIG_DESC_LENGTH), /**/
    0x02,                                 /* bNumInterfaces: control and streaming */
    0x01,                                 /* bConfigurationValue */
    0x00,                                 /* iConfiguration */
    0xC0,                                 /* bmAttributes: self powered */
    0x32,                                 /* MaxPower 100 mA */

    /*---------------------------------------------------------------------------*/

    /* Audio control interface */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    AUDIO_CONTROL_INTERFACE,       /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_CLASS_AUDIO,               /* bInterfaceClass */
    USB_SUBCLASS_AUDIOCONTROL,     /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */

    /* Class specific audio control interface header */
    UAC_HEADER_LENGTH,                /* bLength */
    USB_DESCRIPTOR_TYPE_CS_INTERFACE, /* bDescriptorType */
    UAC_HEADER,                       /* bDescriptorSubtype */
    0x00,                             /* bcdADC: Audio Class 1.0 */
    0x01,                             /**/
    LOBYTE(AUDIO_CONTROL_CS_LENGTH),  /* wTotalLength: header and units */
    HIBYTE(AUDIO_CONTROL_CS_LENGTH),  /**/
    0x01,                             /* bInCollection: one streaming interface */
    AUDIO_STREAMING_INTERFACE,        /* baInterfaceNr(1) */

    /* Input terminal: the microphone */
    UAC_INPUT_TERMINAL_LENGTH,        /* bLength */
    USB_DESCRIPTOR_TYPE_CS_INTERFACE, /* bDescriptorType */
    UAC_INPUT_TERMINAL,               /* bDescriptorSubtype */
    AUDIO_INPUT_TERMINAL_ID,          /* bTerminalID */
    LOBYTE(UAC_TERMINAL_MICROPHONE),  /* wTerminalType */
    HIBYTE(UAC_TERMINAL_MICROPHONE),  /**/
    0x00,                             /* bAssocTerminal */
    AUDIO_CHANNELS,                   /* bNrChannels */
    0x00,                             /* wChannelConfig: mono */
    0x00,                             /**/
    0x00,                             /* iChannelNames */
    0x00,                             /* iTerminal */

    /* Output terminal: the USB stream to the host */
    UAC_OUTPUT_TERMINAL_LENGTH,         /* bLength */
    USB_DESCRIPTOR_TYPE_CS_INTERFACE,   /* bDescriptorType */
    UAC_OUTPUT_TERMINAL,                /* bDescriptorSubtype */
    AUDIO_OUTPUT_TERMINAL_ID,           /* bTerminalID */
    LOBYTE(UAC_TERMINAL_USB_STREAMING), /* wTerminalType */
    HIBYTE(UAC_TERMINAL_USB_STREAMING), /**/
    0x00,                               /* bAssocTerminal */
    AUDIO_INPUT_TERMINAL_ID,            /* bSourceID */
    0x00,                               /* iTerminal */

    /*---------------------------------------------------------------------------*/

    /* Audio streaming interface, alternate setting 0: no bandwidth while not recording */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    AUDIO_STREAMING_INTERFACE,     /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x00,                          /* bNumEndpoints */
    USB_CLASS_AUDIO,               /* bInterfaceClass */
    USB_SUBCLASS_AUDIOSTREAMING,   /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */

    /* Audio streaming interface, alternate setting 1: streaming */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    AUDIO_STREAMING_INTERFACE,     /* bInterfaceNumber */
    0x01,                          /* bAlternateSetting */
    0x01,                          /* bNumEndpoints */
    USB_CLASS_AUDIO,               /* bInterfaceClass */
    USB_SUBCLASS_AUDIOSTREAMING,   /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */

    /* Class specific audio streaming interface */
    0x07,                             /* bLength */
    USB_DESCRIPTOR_TYPE_CS_INTERFACE, /* bDescriptorType */
    UAC_AS_GENERAL,                   /* bDescriptorSubtype */
    AUDIO_OUTPUT_TERMINAL_ID,         /* bTerminalLink */
    0x01,                             /* bDelay: one frame */
    LOBYTE(UAC_FORMAT_PCM),           /* wFormatTag */
    HIBYTE(UAC_FORMAT_PCM),           /**/

    /* Format type I: one channel, 16 bit samples, one sample rate */
    0x0B,                             /* bLength */
    USB_DESCRIPTOR_TYPE_CS_INTERFACE, /* bDescriptorType */
    UAC_FORMAT_TYPE,                  /* bDescriptorSubtype */
    UAC_FORMAT_TYPE_I,                /* bFormatType */
    AUDIO_CHANNELS,                   /* bNrChannels */
    AUDIO_BYTES_PER_SAMPLE,           /* bSubframeSize */
    AUDIO_BYTES_PER_SAMPLE * 8,       /* bBitResolution */
    0x01,                             /* bSamFreqType: one discrete sample rate */
    (AUDIO_SAMPLE_RATE >> 0) & 0xff,  /* tSamFreq */
    (AUDIO_SAMPLE_RATE >> 8) & 0xff,  /**/
    (AUDIO_SAMPLE_RATE >> 16) & 0xff, /**/

    /* Isochronous IN endpoint, audio class 1.0 endpoints have two extra bytes */
    0x09,                         /* bLength */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType */
    AUDIO_EP_DATA_IN,             /* bEndpointAddress */
    0x05,                         /* bmAttributes: isochronous, asynchronous */
    LOBYTE(AUDIO_PACKET_SIZE),    /* wMaxPacketSize */
    HIBYTE(AUDIO_PACKET_SIZE),    /**/
    AUDIO_EP_INTERVAL,            /* bInterval */
    0x00,                         /* bRefresh */
    0x00,                         /* bSynchAddress */

    /* Class specific isochronous endpoint */
    0x07,                            /* bLength */
    USB_DESCRIPTOR_TYPE_CS_ENDPOINT, /* bDescriptorType */
    UAC_EP_GENERAL,                  /* bDescriptorSubtype */
    0x00,                            /* bmAttributes: no sample rate or pitch control */
    0x00,                            /* bLockDelayUnits */
    0x00,                            /* wLockDelay */
    0x00,                            /**/
};

//
// Language ID Descriptor
//

#define USB_LANGID_STRING 0x409 /* 1033 US.S English */
#define USB_LANGID_DESC_LENGTH (4)

/** @brief String descriptor 0, the supported language ids */
static const uint8_t langid_desc[USB_LANGID_DESC_LENGTH] = {
    USB_LANGID_DESC_LENGTH,
    USB_DESCRIPTOR_TYPE_STRING,
    LOBYTE(USB_LANGID_STRING),
    HIBYTE(USB_LANGID_STRING),
};

#endif
