#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["numpy"]
# ///

"""Draw animated demo effects on the GUD display's framebuffer

Switches the framebuffer on and draws a plasma, a tunnel flight and fire, by default
cycling through them, until Ctrl-C. Needs write access to /dev/fbN, which belongs to the
video group.

Run with "uv run tools/fb_animation.py".
"""

import argparse
import os
import sys
import time

import numpy as np
from fb_activate import activate, find_framebuffer, framebuffer_geometry

EFFECTS = ("plasma", "tunnel", "fire")


def plasma(width, height):
    """Overlapping sine waves with cycling colours"""
    y, x = np.mgrid[0:height, 0:width].astype(np.float32)
    cx, cy = width / 2, height / 2

    def frame(t):
        v = (
            np.sin(x * 0.045 + t)
            + np.sin(y * 0.06 + t * 1.3)
            + np.sin((x + y) * 0.03 + t * 0.7)
            + np.sin(np.hypot(x - cx * (1 + np.sin(t * 0.5)), y - cy) * 0.06 - t * 2)
        )
        phase = v * np.pi / 2
        return np.stack(
            (
                0.5 + 0.5 * np.sin(phase + t),
                0.5 + 0.5 * np.sin(phase + t + 2 * np.pi / 3),
                0.5 + 0.5 * np.sin(phase + t + 4 * np.pi / 3),
            ),
            axis=-1,
        )

    return frame


def tunnel(width, height):
    """Flight through a textured tunnel, using precomputed angle and depth tables"""
    y, x = np.mgrid[0:height, 0:width].astype(np.float32)
    dx, dy = x - width / 2, y - height / 2
    radius = np.maximum(np.hypot(dx, dy), 1.0)
    depth = 32.0 * height / radius
    angle = np.arctan2(dy, dx) / np.pi
    shade = np.clip(radius / (height * 0.6), 0.0, 1.0)

    def frame(t):
        u = angle * 4 + t * 0.3
        v = depth + t * 40
        # A chequer pattern that twists along the tunnel
        pattern = (np.floor(u * 2 + np.sin(v * 0.05)) + np.floor(v / 16)) % 2
        glow = 0.6 + 0.4 * np.sin(v * 0.1)
        return np.stack(
            (
                shade * (0.3 + 0.7 * pattern) * glow,
                shade * (0.2 + 0.5 * pattern) * (1 - glow * 0.5),
                shade * (0.6 + 0.4 * (1 - pattern)),
            ),
            axis=-1,
        )

    return frame


def fire(width, height):
    """Heat rising from the bottom row, cooling as it goes"""
    heat = np.zeros((height + 2, width), dtype=np.float32)
    rng = np.random.default_rng()

    def frame(t):
        del t
        heat[-2:] = rng.random((2, width)) ** 3 * 1.6
        below = heat[1:-1] + heat[2:]
        sideways = np.roll(heat[1:-1], 1, axis=1) + np.roll(heat[1:-1], -1, axis=1)
        heat[:-2] = np.clip((below + sideways) / 4 - 0.012, 0.0, None)
        h = heat[:height]
        return np.stack(
            (np.clip(h * 3, 0, 1), np.clip(h * 3 - 1, 0, 1), np.clip(h * 3 - 2, 0, 1)),
            axis=-1,
        )

    return frame


def pack(rgb, bpp, stride):
    """Convert an RGB image of floats from 0 to 1 to the framebuffer's pixel format"""
    height, width, _ = rgb.shape
    r, g, b = (np.clip(rgb[..., i], 0.0, 1.0) for i in range(3))
    if bpp == 16:
        pixels = (
            ((r * 31).astype(np.uint16) << 11)
            | ((g * 63).astype(np.uint16) << 5)
            | (b * 31).astype(np.uint16)
        )
    elif bpp == 32:
        pixels = (
            ((r * 255).astype(np.uint32) << 16)
            | ((g * 255).astype(np.uint32) << 8)
            | (b * 255).astype(np.uint32)
        )
    else:
        sys.exit(f"{bpp} bits per pixel is not supported")
    # Lines can be longer than the visible width
    lines = np.zeros((height, stride // pixels.itemsize), dtype=pixels.dtype)
    lines[:, :width] = pixels
    return lines.astype(pixels.dtype.newbyteorder("<")).tobytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--device", help="framebuffer device (default: the GUD display's framebuffer)"
    )
    parser.add_argument(
        "--effect",
        choices=(*EFFECTS, "all"),
        default="all",
        help="effect to draw (default: all, in turn)",
    )
    parser.add_argument(
        "--seconds",
        type=float,
        default=8,
        help="seconds per effect with --effect all (default 8)",
    )
    parser.add_argument(
        "--fps", type=float, default=30, help="frames per second (default 30)"
    )
    args = parser.parse_args()

    device = args.device or find_framebuffer()
    if device is None:
        sys.exit("No GUD framebuffer found, is the display attached?")
    try:
        activate(device)
    except PermissionError:
        sys.exit(f"No write access to {device}, add yourself to the video group")
    width, height, bpp, stride = framebuffer_geometry(device)
    names = EFFECTS if args.effect == "all" else (args.effect,)
    effects = {name: globals()[name](width, height) for name in names}
    print(f"Drawing {', '.join(names)} on {device} ({width}x{height}), Ctrl-C to stop")

    fd = os.open(device, os.O_WRONLY)
    start = time.monotonic()
    frames = 0
    try:
        while True:
            t = time.monotonic() - start
            name = names[int(t / args.seconds) % len(names)]
            os.pwrite(fd, pack(effects[name](t), bpp, stride), 0)
            frames += 1
            time.sleep(max(0.0, start + frames / args.fps - time.monotonic()))
    except KeyboardInterrupt:
        elapsed = time.monotonic() - start
        print(
            f"\n{frames} frames in {elapsed:.1f} s, {frames / elapsed:.1f} per second"
        )
    finally:
        os.pwrite(fd, bytes(stride * height), 0)
        os.close(fd)


if __name__ == "__main__":
    main()
