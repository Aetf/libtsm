# FindPangoCairo.cmake
#
# CMake support for PangoCairo.
#
find_package(PkgConfig QUIET)
pkg_check_modules(PangoCairo ${PangoCairo_FIND_REQUIRED} ${PangoCairo_FIND_QUIETLY} pangocairo IMPORTED_TARGET)
