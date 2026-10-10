###
### VERBOSE
###

ifeq ($(V),)
VERBOSE=@
else
VERBOSE=
endif

###
### OUTPUT FOLDERS
###

BUILD_DIR=out
BUILD_LIB_DIR=out/lib


###
### TARGETS
###

TARGETS += \
	$(BUILD_DIR)/usb_audio_input_device \
	$(BUILD_DIR)/usb_bulk_device \
	$(BUILD_DIR)/usb_cdc_device \
	$(BUILD_DIR)/usb_keyboard_device \
	$(BUILD_DIR)/usb_mouse_device

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
	$(VERBOSE)mkdir -p $(BUILD_DIR)

$(BUILD_LIB_DIR):
	$(VERBOSE)mkdir -p $(BUILD_LIB_DIR)

$(BUILD_DIR)/%.o : %.c | $(BUILD_DIR)
	@echo "  Compiling $(notdir $<)"
	$(VERBOSE)$(COMPILE.c) $(CFLAGS-$@) -o $@ $<


###
### GENERIC DEPENDS
###

SOURCES := $(shell find src/ -iname *.c)
DEPENDS := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.d)))
-include $(DEPENDS)


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
	$(VERBOSE)ar cr $@ $^


###
### BUILD USB_AUDIO_INPUT DEVICE
###

vpath %.c src/usb_audio_input

CFLAGS += -I src/usb_audio_input

USB_AUDIO_INPUT_SOURCES = \
	usb_audio_input_device.c

USB_AUDIO_INPUT_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_AUDIO_INPUT_SOURCES:.c=.o))
USB_AUDIO_INPUT_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_audio_input_device: $(USB_AUDIO_INPUT_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -lm -o $@


###
### BUILD USB_BULK DEVICE
###

vpath %.c src/usb_bulk

CFLAGS += -I src/usb_bulk

USB_BULK_SOURCES = \
	usb_bulk_device.c

USB_BULK_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_BULK_SOURCES:.c=.o))
USB_BULK_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_bulk_device: $(USB_BULK_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -o $@


###
### BUILD USB_CDC DEVICE
###

vpath %.c src/usb_cdc

CFLAGS += -I src/usb_cdc

USB_CDC_SOURCES = \
	usb_cdc_device.c

USB_CDC_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_CDC_SOURCES:.c=.o))
USB_CDC_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_cdc_device: $(USB_CDC_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -o $@


###
### BUILD USB_GUD_DISPLAY DEVICE, NOT PART OF ALL SINCE IT NEEDS SDL3 (libsdl3-dev)
###

vpath %.c src/usb_gud_display

CFLAGS += -I src/usb_gud_display

USB_GUD_DISPLAY_SOURCES = \
	usb_gud_display_device.c \
	gud_display_window.c

USB_GUD_DISPLAY_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_GUD_DISPLAY_SOURCES:.c=.o))
USB_GUD_DISPLAY_OBJECTS += $(LIBRARY_ARCHIVE)

CFLAGS-$(BUILD_DIR)/gud_display_window.o = $(shell pkg-config --cflags sdl3)

$(BUILD_DIR)/gud_display_window.o: | sdl3_check

sdl3_check:
	@pkg-config --exists sdl3 || { echo "The GUD display needs SDL3, install libsdl3-dev"; exit 1; }

gud_display: $(BUILD_DIR)/usb_gud_display_device

$(BUILD_DIR)/usb_gud_display_device: $(USB_GUD_DISPLAY_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group $(shell pkg-config --libs sdl3) -o $@


###
### BUILD USB_KEYBOARD DEVICE
###

vpath %.c src/usb_keyboard

CFLAGS += -I src/usb_keyboard

USB_KEYBOARD_SOURCES = \
	usb_keyboard_device.c

USB_KEYBOARD_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_KEYBOARD_SOURCES:.c=.o))
USB_KEYBOARD_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_keyboard_device: $(USB_KEYBOARD_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -lm -o $@


###
### BUILD USB_MOUSE DEVICE
###

vpath %.c src/usb_mouse

CFLAGS += -I src/usb_mouse

USB_MOUSE_SOURCES = \
	usb_mouse_device.c

USB_MOUSE_OBJECTS = $(addprefix $(BUILD_DIR)/, $(USB_MOUSE_SOURCES:.c=.o))
USB_MOUSE_OBJECTS += $(LIBRARY_ARCHIVE)


$(BUILD_DIR)/usb_mouse_device: $(USB_MOUSE_OBJECTS) | $(BUILD_DIR)
	@echo "  Linking $(notdir $<) to $(notdir $@)"
	$(VERBOSE)$(LINK.o) -Wl,--start-group $^ -Wl,--end-group -lm -o $@


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


###
### CHECK THAT THE PYTHON TOOLS ARE FORMATTED AND LINT FREE WITH RUFF
###

check-python:
	@echo "  Checking formatting of tools/"
	$(VERBOSE)uvx ruff format --check tools
	@echo "  Linting tools/"
	$(VERBOSE)uvx ruff check tools
