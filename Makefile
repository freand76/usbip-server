PROG=Test

SRC_FILES = \
	Test.cc \
	UsbIpServer.cc \
	UsbDevice.cc \
	NetworkUtil.cc

CPP=g++

CFLAGS=-O2 -Werror -Wextra -Wall -I. -std=c++11
CFLAGS+=-ggdb
CFLAGS+=$(shell wx-config --cxxflags)
LDFLAGS=
LDFLAGS+=$(shell wx-config --libs)
OBJDIR=.x86_64
OBJ_FILES=$(addprefix $(OBJDIR)/,$(SRC_FILES:.cc=.o))
PROG:=$(addprefix $(OBJDIR)/,$(PROG))

all: $(OBJDIR) $(PROG)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o : src/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(PROG) : $(OBJ_FILES)
	$(CPP) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR)
