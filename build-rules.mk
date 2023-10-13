CPP=g++

LDFLAGS=
CFLAGS=

CFLAGS+=-pedantic

# Developer flags (gdb)
CFLAGS+=-ggdb

# Developer flags (sanitize)
CFLAGS+=-fsanitize=address
LDFLAGS+=-lasan

CFLAGS+=-O2 -std=c++11 -Werror -Wextra -Wall -I.
CFLAGS+=-I./src/server \
	-I./src/usb \
	-I./src/device \
	-I./src/util \
	-I./src/log \
	-I./src/app

LDFLAGS+=-lpthread


OBJDIR=.x86
$(shell mkdir -p $(OBJDIR))

OBJ_FILES=$(addprefix $(OBJDIR)/,$(SRC_FILES:.cc=.o))

PROG:=$(addprefix $(OBJDIR)/,$(PROG))

$(OBJDIR)/%.o : src/server/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/usb/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/device/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/util/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/log/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o : src/app/%.cc
	$(CPP) -c $(CFLAGS) $^ -o $@

$(PROG) : $(OBJ_FILES)
	$(CPP) $(LDFLAGS) $^ -o $@ $(LDFLAGS)
