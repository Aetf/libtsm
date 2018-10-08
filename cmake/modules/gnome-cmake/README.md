# CMake modules for GNOME libraries and tools

This is a collection of CMake modules to support using GNOME libraries
and tools from CMake-based projects.  These modules were initially
intended for use from Vala projects, but many can be used for other
languages as well.

There are currently modules for:

* ATK
* Cairo
* GDK3
* GDKPixbuf
* GIO
* GLib
* GObject
* GObjectIntrospection
* GTK3
* Pango
* Soup
* Vala
* Valadoc

Packages for libraries create imported targets with the same name as
the pkg-config file.

Several of the modules also contain functions for utilizing them; for
example, the Vala contains a function to use the Vala compiler,
GObjectIntrospection contains a function to generate a typelib from a
GIR, and so on.

This is still very much a work in progress, but it is already fairly
useful.
