PROG=TestApp

SRC_FILES = \
	UsbIpServer.cc \
	UsbIpDevice.cc \
	UsbDevice.cc \
	UsbConfiguration.cc \
	UsbInterface.cc \
	UsbEndpoint.cc \
	UsbUtil.cc \
	NetworkUtil.cc \
	Verbose.cc \
	HidMouse.cc \
	TestApp.cc

include build-rules.mk

all: $(PROG)

clean:
	rm -rf $(OBJDIR)
