###
### OUTPUT FOLDERS
### 

BUILD_DIR=out
BUILD_LIB_DIR=out/lib


###
### TARGETS
### 

TARGETS += $(BUILD_DIR)/usb_cdc_device

all: $(TARGETS)


###
### COMMON FLAGS AND RULES
### 

CFLAGS += -std=c99 \
	-pedantic -Wall -Werror -Wextra \
	-Wdouble-promotion -Wstrict-prototypes -Wcast-qual \
	-Wmissing-prototypes -Winit-self -Wpointer-arith -Wshadow -MMD -MP -O3 \
	-fno-var-tracking-assignments -ffunction-sections -fdata-sections
CFLAGS += -D_GNU_SOURCE

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR) 

$(BUILD_LIB_DIR):
	@mkdir -p $(BUILD_LIB_DIR)

$(BUILD_DIR)/%.o : %.c | $(BUILD_DIR)
	@echo "  Compiling $(notdir $<)"
	@$(COMPILE.c) $(CFLAGS-$@) -o $@ $<


###
### BUILD USBIP_SERVER LIBRARY 
### 

vpath %.c src/lib

CFLAGS += -Isrc/lib -Iinclude

LIBRARY_SOURCES = \
	tcpip_server.c \
	usbip_device.c \
	usbip_protocol.c \
	usbip_server.c

LIBRARY_OBJECTS = $(addprefix $(BUILD_DIR)/, $(LIBRARY_SOURCES:.c=.o))
LIBRARY_NAME = libusbip_server.a
LIBRARY_ARCHIVE = $(addprefix $(BUILD_LIB_DIR)/, $(LIBRARY_NAME))

lib: $(LIBRARY_ARCHIVE)

$(LIBRARY_ARCHIVE): $(LIBRARY_OBJECTS) | $(BUILD_LIB_DIR)
	@echo "  Creating library $(notdir $@)"
	@ar cr $@ $^


###
### BUILD USB_CDC DEVICE
### 

vpath %.c src/usb_cdc

CFLAGS += -I src/usb_cdc

USB_CDC_SOURCES = \
	usb_cdc_device.c

CDC_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_CDC_SOURCES:.c=.o))
CDC_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_cdc_device: $(CDC_OBJECTS) | $(BUILD_DIR) 
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	@$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -o $@


###
### GENERIC DEPENDS
### 

SOURCES := $(sort $(foreach path,${VPATH},$(wildcard $(path)/*.c)))
DEPENDS := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.d)))

-include $(DEPENDS)

###
### CLEAN
### 

clean:
	rm -rf $(BUILD_DIR)


###
### FORMAT SOURCE FILES WITH CLANG FORMAT
### 

format:
	@find src/. -iname *.h -o -name *.c | xargs clang-format -i
	@find include/. -iname *.h  | xargs clang-format -i	
	git status


