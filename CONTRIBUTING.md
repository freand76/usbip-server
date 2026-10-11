# Contributing

Thanks for helping! New example devices, fixes to the USB/IP or chapter 9 handling, and
better documentation are all welcome.

## Questions and Ideas

Ask questions and share devices you built in
[Discussions](https://github.com/freand76/usbip-server/discussions). Use
[issues](https://github.com/freand76/usbip-server/issues) for bugs, and include:

- the device program and how you attached it;
- the device's output and `journalctl -k` from the attach;
- your distribution and kernel version (`uname -r`).

## Pull Requests

- Keep each commit to one change. A fix and a cleanup next to it are two commits.
- Write commit messages that explain **why** the change is needed, in a short
  imperative subject line and a few lines of body. The diff already shows how.
- Run the checks that CI runs:

  ```
  $ make && make gud_display
  $ make format          # or make check-format to only check
  $ make check-python    # if you changed tools/
  ```

- Test with the real kernel: attach the device with `usbip attach -r localhost -b 1-1`
  and check that its driver binds and works.

## New Example Devices

A new device needs:

- a folder `src/usb_<name>/` with `usb_<name>_device.c` and its descriptors, and an
  entry in `TARGETS` in the makefile so `make` builds `out/usb_<name>_device`. A device
  that needs extra libraries gets its own make target, like `make gud_display`, so the
  default build stays dependency free;
- vendor id `0xfa00` with an unused product id, unless a kernel driver only binds
  by id;
- a page `docs/usb_<name>_device.md`, listed in `docs/index.md` and in the
  README's examples table.

By contributing you agree that your contribution is licensed under the
[MIT license](LICENSE) of the project.
