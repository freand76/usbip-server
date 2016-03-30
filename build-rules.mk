CPP=g++
CFLAGS=-O2 -Werror -Wextra -Wall -I.
CFLAGS+=-I./src/server \
	-I./src/usb \
	-I./src/util \
	-I./src/log

CFLAGS+=-ggdb
CFLAGS+=$(shell wx-config --cxxflags)

LDFLAGS=
LDFLAGS+=$(shell wx-config --libs)

OBJDIR=.x86
$(shell mkdir -p $(OBJDIR))

OBJ_FILES=$(addprefix $(OBJDIR)/,$(SRC_FILES:.cc=.o))

PROG:=$(addprefix $(OBJDIR)/,$(PROG))

$(OBJDIR)/%.o : src/server/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/usb/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/util/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/log/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/app/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(PROG) : $(OBJ_FILES)
	$(CPP) $(LDFLAGS) $^ -o $@ $(LDFLAGS)
