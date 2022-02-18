find_package(PkgConfig QUIET)
pkg_check_modules(Cairo ${Cairo_FIND_REQUIRED} ${Cairo_FIND_QUITELY} cairo IMPORTED_TARGET)
