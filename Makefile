PROG=HidMouse

include common-src.mk

SRC_FILES += HidMouse.cc

include build-rules.mk

all: $(PROG)

clean:
	rm -rf $(OBJDIR)
