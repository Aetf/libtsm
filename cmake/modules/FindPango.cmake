find_package(PkgConfig QUIET)
pkg_check_modules(Pango ${Pango_FIND_REQUIRED} ${Pango_FIND_QUIETLY} pango IMPORTED_TARGET)
