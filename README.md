# TSM - Terminal Emulator State Machine

![Build Status](https://github.com/aetf/libtsm/actions/workflows/meson.yml/badge.svg?branch=develop)

TSM is a state machine for DEC VT100-VT520 compatible terminal emulators. It
tries to support all common standards while keeping compatibility to existing
emulators like xterm, gnome-terminal, konsole, ...

This is a personal modified version. For more information, please refer to its original [README](README).

## Added feature
+ More color palettes:
    * soft-black
    * [base16](https://github.com/chriskempson/base16-default-schemes){-light,-dark}
    * solarized{,-black,-white}
    * custom: set via API
+ Support underline/italic rendering (with [a patched version of kmscon](https://github.com/Aetf/kmscon))
+ Support 24-bit true color
+ Support Ctrl + Arrow keys
+ Support custom title using OSC
+ Bug fixes:
    * [Response to 'CSI c' contains random bytes][91335]
    * [Fix invalid cpr values](https://github.com/Aetf/libtsm/pull/2)

[91335]: https://bugs.freedesktop.org/show_bug.cgi?id=91335

## Build
```bash
meson setup build
cd build
meson compile
meson install
```

### Build options
Options may be supplied when configuring meson:
```bash
meson -Dtests=true -Dextra_debug=true -Dgtktsm=true
```
The following options are available:

|Name | Description | Default |
|:---:|:---|:---:|
| tests | Whether build the test suite | ON |
| extra_debug | Whether to enable several non-standard debug options | OFF |
| gtktsm | Whether to build the gtktsm example. This is linux-only as it uses epoll and friends. Therefor is disabled by default. | OFF |

## Dependencies
### Required
- [meson](https://mesonbuild.com) >= 3.5
### Optional
- `xkbcommon-keysyms.h` from xkbcommon (Optional. Will use private copy if not found.)
## Test suite
- [check](https://libcheck.github.io/check/) >= 0.9.10 (For the test suite)
## Gtktsm
- gtk3
- cairo
- pango
- xkbcommon
