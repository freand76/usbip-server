#!/usr/bin/env python3

"""Play an audio input, like the usb_audio_input example device, on the speakers

Works with PulseAudio and with PipeWire, through pactl. The input is played until
Ctrl-C. When it disappears, for example when the device is detached, playing stops and
starts again when the input comes back.
"""

import argparse
import signal
import subprocess
import sys
import time

DEFAULT_SOURCE = "Virtual_Audio_Input"
# USB/IP delivers audio in 1 ms packets with some network delay on top, much lower
# latencies make the sound server run out of samples and the sound break up
DEFAULT_LATENCY_MS = 30


def pactl(*args, must_succeed=True):
    result = subprocess.run(
        ["pactl", *args], capture_output=True, text=True, check=False
    )
    if must_succeed and result.returncode != 0:
        sys.exit(f"pactl {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def list_names(kind):
    """Names of the sources or sinks, leaving out the monitors of the outputs"""
    names = [line.split("\t")[1] for line in pactl("list", "short", kind).splitlines()]
    return [name for name in names if not name.endswith(".monitor")]


def find_source(pattern):
    matches = [
        name for name in list_names("sources") if pattern.lower() in name.lower()
    ]
    return matches[0] if matches else None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--source",
        default=DEFAULT_SOURCE,
        help=f"part of the input name (default {DEFAULT_SOURCE})",
    )
    parser.add_argument("--sink", help="output name (default: the default output)")
    parser.add_argument(
        "--latency-ms",
        type=int,
        default=DEFAULT_LATENCY_MS,
        help=f"loopback latency (default {DEFAULT_LATENCY_MS})",
    )
    parser.add_argument(
        "--list", action="store_true", help="list the inputs and outputs, then exit"
    )
    args = parser.parse_args()

    if args.list:
        print("Inputs:\n  " + "\n  ".join(list_names("sources")))
        print("Outputs:\n  " + "\n  ".join(list_names("sinks")))
        return

    # Ctrl-C and kill both end the loopback, also when started in the background, where
    # SIGINT is ignored by default
    signal.signal(signal.SIGINT, signal.default_int_handler)
    signal.signal(signal.SIGTERM, signal.default_int_handler)

    module = None
    try:
        while True:
            source = find_source(args.source)
            if source is None:
                print(f"Waiting for an input matching '{args.source}'")
                while source is None:
                    time.sleep(0.5)
                    source = find_source(args.source)

            # Without source_dont_move the sound server moves the loopback to another
            # input when this one disappears, and could play a real microphone
            module_args = [
                f"source={source}",
                f"latency_msec={args.latency_ms}",
                "source_dont_move=true",
            ]
            if args.sink:
                module_args.append(f"sink={args.sink}")
            module = pactl("load-module", "module-loopback", *module_args).strip()
            print(f"Playing {source} on {args.sink or 'the default output'}")
            print("Ctrl-C to stop")

            while source in list_names("sources"):
                time.sleep(0.5)

            # PulseAudio removes the loopback by itself, PipeWire keeps it
            pactl("unload-module", module, must_succeed=False)
            module = None
            print("The input disappeared")
    except KeyboardInterrupt:
        print("\nStopped")
    finally:
        if module is not None:
            pactl("unload-module", module, must_succeed=False)


if __name__ == "__main__":
    main()
