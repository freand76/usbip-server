PROG=TestApp

SRC_FILES = \
	UsbIpServer.cc \
	UsbDevice.cc \
	UsbConfiguration.cc \
	UsbInterface.cc \
	UsbEndpoint.cc \
	UsbUtil.cc \
	NetworkUtil.cc \
	TestApp.cc

include build-rules.mk

all: $(PROG)

clean:
	rm -rf $(OBJDIR)
