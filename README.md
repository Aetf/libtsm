# TSM - Terminal Emulator State Machine

TSM is a state machine for DEC VT100-VT520 compatible terminal emulators. It
tries to support all common standards while keeping compatibility to existing
emulators like xterm, gnome-terminal, konsole, ..

This is a personal modified version. For more information, please refer to its original [README](README).

## Added features
+ Configurable color palette
+ Support underline/italic rendering (with [a patched version of kmscon](https://github.com/Aetf/kmscon))
+ Support 24-bit true color
+ Support Ctrl + Arrow keys
+ Support custom title using OSC
+ Bug fixes:
    * [Repsonse to 'CSI c' contains random bytes][91335]
    * [Fix invalid cpr values](https://github.com/Aetf/libtsm/pull/2)

[91335]: https://bugs.freedesktop.org/show_bug.cgi?id=91335
