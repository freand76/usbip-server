# Audio Input

A USB Audio Class 1 microphone streaming over an isochronous endpoint. Linux binds its
`snd-usb-audio` driver to it and it shows up as a sound card and as a microphone in the
desktop sound settings. While something records from it, it plays an alien pulse once a
second: a warbling chirp sweeping from 220 Hz to 1760 Hz while it fades out.

| | |
|---|---|
| Program | `out/usb_audio_input_device` |
| Source | [src/usb_audio_input/](../src/usb_audio_input/) |
| USB id | `fa00:0004` |
| Interfaces | audio control (class `0x01`, subclass `0x01`), audio streaming (class `0x01`, subclass `0x02`) with alternate settings 0 (idle) and 1 (streaming) |
| Endpoint | `0x81` isochronous IN, asynchronous, one 96 byte packet per millisecond |
| Format | 48 kHz, 16 bit, mono |

It is a class compliant device, so the audio driver needs no special vendor or product
id.

## Try It

```
$ make
$ out/usb_audio_input_device &
$ sudo usbip attach -r localhost -b 1-1
$ arecord -l
card 2: Input [Virtual Audio Input], device 0: USB Audio [USB Audio]
```

Record five seconds and play them back:

```
$ arecord -D plughw:CARD=Input -f S16_LE -r 48000 -c 1 -d 5 alien.wav
$ aplay alien.wav
```

Or play it live on the speakers with [tools/audio_loopback.py](../tools/audio_loopback.py),
which works with both PulseAudio and PipeWire. Start it before or after attaching the
device; it plays the input until Ctrl-C, stops when the device is detached and starts
again when it is attached:

```
$ tools/audio_loopback.py
$ tools/audio_loopback.py --list                     # inputs and outputs
$ tools/audio_loopback.py --source Mic1 --sink Speaker --latency-ms 50
```

The default latency is 30 ms. Much lower latencies make the sound break up: the audio
arrives in 1 ms packets with some USB/IP delay on top, and the sound server runs out of
samples between them.

Detach with `sudo usbip detach -p 0`.

## What You See

```
USB Audio Input Device Activated
USBIPServer: Connected
USBIPProtocol: USBIP_CMD_REQ_DEV_IMPORT '1-1'
Recording started
Recording stopped
```

- `Recording started` and `stopped` follow the host selecting alternate setting 1 and 0
  of the streaming interface. The desktop sound server briefly opens a new microphone
  when it appears, so a few short start and stop pairs right after the attach are
  normal.
- When recording starts the kernel logs `usb usbN: Not yet implemented`. The virtual
  host controller does not implement frame numbers, which the audio driver asks for,
  and the message is harmless.

## How It Works

- The descriptors in
  [usb_audio_input_descriptors.h](../src/usb_audio_input/usb_audio_input_descriptors.h)
  describe a microphone input terminal feeding a USB streaming output terminal, and a
  type I PCM format with one sample rate.
- `iso_in_data()` is the isochronous IN callback. The server calls it once per 1 ms
  service interval as the host reads the endpoint, and it returns 48 samples each time.
  The server completes transfers in real time, so the stream runs at exactly 48 kHz.
- `next_sample()` generates the pulse: an exponential frequency sweep with a 30 Hz
  warble, a 10 ms attack and a fade to silence after 400 ms, repeated every second.
- `control_data()` follows `SET_INTERFACE` to know when recording starts, and restarts
  the pulse then, so every recording begins with a pulse.

## Prototyping

Start here for any streaming device: microphones, sound cards, sensors that sample
continuously. Change the sample rate or channel count in the descriptors and the packet
size follows; return the samples of your own signal from `iso_in_data()`.

## Troubleshooting

- `arecord: main: audio open error: Device or resource busy`: another program, often
  the sound server, has the card open. Record through it instead, with
  `tools/audio_loopback.py` or by choosing "Virtual Audio Input" as the microphone.
- The sound breaks up in a loopback: raise the latency, see above.
