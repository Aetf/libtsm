find_package(PkgConfig QUIET)
pkg_check_modules(GTK3 ${GTK3_FIND_REQUIRED} ${GTK3_FIND_QUIETLY} gtk+-3.0 IMPORTED_TARGET)
