# libtsm Release News

## CHANGES WITH 4.3.0:
### New features
* Add OSC 4, 10 and 11 support (only read color, not set it) https://github.com/Aetf/libtsm/pull/55
* Add a new API, tsm_screen_selection_word() to select a word at a given position https://github.com/Aetf/libtsm/pull/54

## CHANGES WITH 4.2.0:
### New features
* Add shift arrow keys by @michael-oberpriller in https://github.com/Aetf/libtsm/pull/46
* Convert to Meson by @kdj0c in https://github.com/Aetf/libtsm/pull/45

### Bug fixes
* Fix github CI, after renaming develop to main by @kdj0c in https://github.com/Aetf/libtsm/pull/47
* Meson: fix version-script by @kdj0c in https://github.com/Aetf/libtsm/pull/49
* Readme.md: Fix build status branch name by @kdj0c in https://github.com/Aetf/libtsm/pull/50
* Fix CI on MacOS by @kdj0c in https://github.com/Aetf/libtsm/pull/52


## CHANGES WITH 4.1.0:
### New features!
 * Make backspace key configurable (#23)
 * Add mouse support (#29)
 * Add getters for scrollback buffer line count and position (#32)
 * Add new VGA palette, same as kernel VT (#38)
 * Add ECMA-48 CSI sequences `\E[nE` and `\E[nF` (#39)

### Bug fixes
 * Rewrite of tsm_screen_selection_copy (#36)
 * Fix a memleak in tsm_screen_unref (#35)
 * Check for nullptr in tsm_vte_get_flags (#31)
 * Fix DECRQM SRM request (#30)
 * Fix build on macOS (#24)
 * Fix wrong background color of new cells after resize (#21)
 * Fix path in pkg-config file
