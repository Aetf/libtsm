# TSM - Terminal Emulator State Machine

[![Build Status](https://travis-ci.com/Aetf/libtsm.svg?branch=master)](https://travis-ci.com/Aetf/libtsm)

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
    * [Repsonse to 'CSI c' contains random bytes][91335]
    * [Fix invalid cpr values](https://github.com/Aetf/libtsm/pull/2)

[91335]: https://bugs.freedesktop.org/show_bug.cgi?id=91335

## Build
```bash
mkdir build && cd build
cmake ..
make
make install
```

### Build options
Options may be supplied when configuring cmake:
```bash
cmake -DOPTION1=VALUE1 -DOPTION2=VALUE2 ..
```
The following options are available:

|Name | Description | Default |
|:---:|:---|:---:|
| BUILD_SHARED_LIBS | Whether to build as a shared library | ON |
| BUILD_TESTING | Whether to build test suits | OFF |
| ENABLE_EXTRA_DEBUG | Whether to enable several non-standard debug options. | OFF |
| BUILD_GTKTSM | Whether to build the gtktsm example. This is linux-only as it uses epoll and friends. Therefore is disabled by default. | OFF |

### Dependencies

- [cmake](https://cmake.org) >= 3.5
- `xkbcommon-keysyms.h` from xkbcommon (Optional. Will use private copy if not found.)

The test suits needs:

- [check](https://libcheck.github.io/check/) >= 0.9.10

The gtktsm example needs:

- gtk3
- cairo
- pango
- xkbcommon
