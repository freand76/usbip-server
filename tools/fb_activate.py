#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///

"""Switch the framebuffer of the GUD display on

The kernel only shows a DRM device's framebuffer, /dev/fbN, once its mode has been set.
Programs like ffmpeg only write pixels, so this sets the current mode again. Needs write
access to /dev/fbN, which belongs to the video group.

Run with "uv run tools/fb_activate.py".
"""

import argparse
import fcntl
import struct
import sys
from pathlib import Path

FBIOGET_VSCREENINFO = 0x4600
FBIOPUT_VSCREENINFO = 0x4601
FB_VAR_SCREENINFO_SIZE = 160
FB_VAR_ACTIVATE_OFFSET = 84
FB_ACTIVATE_FORCE = 128

DEFAULT_NAME = "gud"


def find_framebuffer(name=DEFAULT_NAME):
    """Device path of the first framebuffer whose name starts with name, or None"""
    for fb in sorted(Path("/sys/class/graphics").glob("fb[0-9]*")):
        if (fb / "name").read_text().strip().startswith(name):
            return f"/dev/{fb.name}"
    return None


def framebuffer_geometry(device):
    """Width, height, bits per pixel and bytes per line of a framebuffer"""
    sysfs = Path("/sys/class/graphics") / Path(device).name
    width, height = (int(v) for v in (sysfs / "virtual_size").read_text().split(","))
    bpp = int((sysfs / "bits_per_pixel").read_text())
    stride = int((sysfs / "stride").read_text())
    return width, height, bpp, stride


def activate(device):
    """Set the framebuffer's current mode again, which switches the display on"""
    with open(device, "r+b", buffering=0) as fb:
        var = bytearray(
            fcntl.ioctl(fb, FBIOGET_VSCREENINFO, bytes(FB_VAR_SCREENINFO_SIZE))
        )
        # Apply the mode now, even though it did not change
        struct.pack_into("I", var, FB_VAR_ACTIVATE_OFFSET, FB_ACTIVATE_FORCE)
        fcntl.ioctl(fb, FBIOPUT_VSCREENINFO, var)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--device", help="framebuffer device (default: the GUD display's framebuffer)"
    )
    args = parser.parse_args()

    device = args.device or find_framebuffer()
    if device is None:
        sys.exit("No GUD framebuffer found, is the display attached?")
    try:
        activate(device)
    except PermissionError:
        sys.exit(f"No write access to {device}, add yourself to the video group")
    width, height, bpp, _ = framebuffer_geometry(device)
    print(f"{device}: {width}x{height}, {bpp} bits per pixel, switched on")


if __name__ == "__main__":
    main()
