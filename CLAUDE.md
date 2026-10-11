g# CLAUDE.md

A USB/IP server for prototyping USB devices in user space. A device program serves its
descriptors and endpoint callbacks over USB/IP on TCP port 3240, and the Linux
`vhci_hcd` driver attaches it as a real USB device.

## Commits

- One commit per single-purpose change. Don't bundle unrelated fixes, even small ones.
- Commit messages are tight and answer **why**: what was wrong or missing for the
  user, not how the code changes. Imperative subject line, short body.
- **Never** add a `Co-Authored-By` trailer or any other attribution line.
- Don't amend a pushed commit unless asked. A follow-up fix is a new commit.
- Commit only when asked, or as part of a task that asks for commits. Push only when
  asked, and ask before a force push.
- Never commit the untracked files in the root.

## Layout

- `include/usbip_server_api.h`: the public API, documented with Doxygen. Devices
  implement `usbip_device_t` callbacks: control, endpoint data, `iso_in_callback`,
  `reset_callback`.
- `include/descriptor_utils.h`: descriptor constants and helpers.
- `src/lib/`: the library.
  - `tcpip_server.c`: the socket and `ppoll` wait with an eventfd wake-up.
  - `usbip_protocol.c`: USB/IP commands, the pending URB queue, and interrupt and
    isochronous pacing.
  - `usbip_device.c`: chapter 9 standard requests and `usbip_device_transmit`.
  - `usbip_server.c`: the server thread and signals.
- `src/usb_*/`: the example devices, built to `out/usb_*_device`.
- `tools/`: Python helpers, run with `uv run tools/<name>.py`. Each one is a PEP 723
  script with its dependencies inline.
- `udev/`: udev rules shipped with the repository.
- `docs/`: one page per example device, published with GitHub Pages from `main`
  `/docs` at https://freand76.github.io/usbip-server/. Links from the pages into the
  repository must be full `https://github.com/freand76/usbip-server/blob|tree/main/...`
  URLs, since the site only contains `docs/`. Links between pages are relative `.md`
  links. When you add a device, add a page and add it to `docs/index.md` and the
  README table.

## Build and Check

- `make` builds the library and all examples except the GUD display. CI runs only
  `make`, so it must not need extra packages.
- `make gud_display` builds the GUD display, which needs SDL3 (`libsdl3-dev`). Keep it
  out of `all`.
- `make format` runs clang-format on the C code. Run it before committing C changes.
- `make check-python` runs `uvx ruff format --check` and `uvx ruff check` on `tools/`.
- After pushing, check CI with `gh run list`.

## Testing with the Real Kernel

- Start a device, then `usbip attach -r localhost -b 1-1`, and detach with
  `usbip detach -p 0`. No sudo is needed on this machine.
- The server binds to 127.0.0.1 by default (`USBIP_BIND_ADDRESS` changes it).
- Example devices use vendor id `0xfa00`. Devices that a kernel driver only binds by
  id use a real id instead: the GUD display is `1d50:614d`.

## USB Rules to Keep

- The device is high speed. bMaxPacketSize0 is 64, bulk packets are 512, and
  bInterval is an exponent.
- USB/IP header fields are big-endian. The setup packet and descriptors are
  little-endian.
- IN requests with no data wait in the pending URB queue, which acts as a NAK.
  Callbacks run outside the library's critical sections.
- The library supports at most 128 isochronous packets per URB, and buffers of at
  most 64 KiB.
