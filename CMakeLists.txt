cmake_minimum_required(VERSION 3.5)

project(libtsm
    LANGUAGES C
    VERSION 4.0.2
)

# Some meta data
## TODO: merge into project above after we require cmake 3.12
set(PROJECT_HOMEPAGE_URL "https://github.com/Aetf/libtsm")
## TODO: merge into project above after we require cmake 3.10
set(PROJECT_DESCRIPTION "terminal-emulator state machine")

#---------------------------------------------------------------------------------------
# Initial setups
#---------------------------------------------------------------------------------------
# Ensure out-of-source build
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Usage: mkdir build; cmake ..")
endif()

# Include utilities
include(cmake/Utilities.cmake)
# For feature_summary
include(FeatureSummary)

# Extra build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
include(cmake/BuildTypes.cmake)

#---------------------------------------------------------------------------------------
# Options
#---------------------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build as shared libraries" ON)
add_feature_info(BUILD_SHARED_LIBS BUILD_SHARED_LIBS "build as shared libraries")

option(BUILD_TESTING "Whether to build test suite in default target" OFF)
add_feature_info(BUILD_TESTING BUILD_TESTING "build unit tests")

# We enable a lot of debug options by default, so this option is really only for
# extended developer debug modes.
option(ENABLE_EXTRA_DEBUG "Whether to enable several non-standard debug options." OFF)
add_feature_info(ENABLE_EXTRA_DEBUG ENABLE_EXTRA_DEBUG "enable additional non-standard debug options")

# Whether to our gtktsm example. This is linux-only as it uses epoll and friends.
# Therefore, it's disabled by default.
option(BUILD_GTKTSM "Whether to build the gtktsm example" OFF)
add_feature_info(BUILD_GTKTSM BUILD_GTKTSM "build the gtktsm example, it requires gtk+-3 and friends and is linux-only.")

#---------------------------------------------------------------------------------------
# Find packages
#---------------------------------------------------------------------------------------
# Additional find modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# GNU filesystem layout
include(GNUInstallDirs)

# We need xkbcommon for keysym definitions. If it's not found, we use our own
# private copy of xkbcommon-keysyms.h.
find_package(XKBCommon COMPONENTS KeySyms)
set_package_properties(XKBCommon PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Needed for keysym definitions. Will use private copy if not found."
)

# Optionally, look for gtk+-3 and friends for gtktsm
if(BUILD_GTKTSM)
    find_package(GTK3)
    set_package_properties(GTK3 PROPERTIES
        TYPE REQUIRED
        PURPOSE "For gtktsm example"
    )

    find_package(Cairo)
    set_package_properties(Cairo PROPERTIES
        TYPE REQUIRED
        PURPOSE "For gtktsm example"
    )

    find_package(Pango)
    set_package_properties(Pango PROPERTIES
        TYPE REQUIRED
        PURPOSE "For gtktsm example"
    )

    find_package(PangoCairo)
    set_package_properties(PangoCairo PROPERTIES
        TYPE REQUIRED
        PURPOSE "For gtktsm example"
    )

    find_package(XKBCommon COMPONENTS XKBCommon)
    set_package_properties(XKBCommon PROPERTIES
        TYPE REQUIRED
        PURPOSE "For gtktsm example"
    )
endif()

if(BUILD_TESTING)
    enable_testing()
    find_package(Check)
    set_package_properties(Check PROPERTIES
        TYPE REQUIRED
        PURPOSE "For testing"
    )
endif()

feature_summary(INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)

#---------------------------------------------------------------------------------------
# Set compiler options and features
#---------------------------------------------------------------------------------------
# Only set compile options after any inclusion of third party code
include(cmake/CompileOptions.cmake)

# Pass infomation to config header
if(XKBCommon_KeySyms_FOUND)
    set(BUILD_HAVE_XKBCOMMON ON)
endif()
if(ENABLE_EXTRA_DEBUG)
    set(BUILD_ENABLE_DEBUG ON)
endif(ENABLE_EXTRA_DEBUG)
configure_file(src/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

#---------------------------------------------------------------------------------------
# Put code together
#---------------------------------------------------------------------------------------
# Venderoized external code
add_subdirectory(external)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(test)
endif(BUILD_TESTING)

#---------------------------------------------------------------------------------------
# Installation of other files
#---------------------------------------------------------------------------------------
# pkgconfig file for backward compatibility
configure_file(etc/libtsm.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libtsm.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libtsm.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

#Export the targets to a script
set(INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/libtsm)
install(EXPORT tsm-targets
    FILE
    libtsm-targets.cmake
    NAMESPACE
    libtsm::
    DESTINATION
    ${INSTALL_CMAKEDIR}
)

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libtsm-config-version.cmake
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/etc/libtsm-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libtsm-config.cmake
    PATH_VARS
        INSTALL_CMAKEDIR
    INSTALL_DESTINATION
        ${INSTALL_CMAKEDIR}
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libtsm-config-version.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libtsm-config.cmake
    DESTINATION ${INSTALL_CMAKEDIR}
)
