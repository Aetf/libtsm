# Set compiler flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# analogous to AC_USE_SYSTEM_EXTENSIONS in configure.ac
add_definitions(-D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE)

# copied from Autoconf's AC_SYS_LARGEFILE
if(NOT WIN32)
    add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
endif(NOT WIN32)

# Set compiler flags for warnings
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # using Clang or AppleClang

    # reasonable and standard
    add_compile_options_with_check(-Weverything)
    add_compile_options_with_check(-Werror)
    add_compile_options_with_check(-Wfatal-errors)

    add_compile_options_with_check(-Wold-style-cast)
    # helps catch hard to track down memory errors
    add_compile_options_with_check(-Wnon-virtual-dtor)
    # warn for potential performance problem casts
    add_compile_options_with_check(-Wcast-align)
    # warn if you overload (not override) a virtual function
    add_compile_options_with_check(-Woverloaded-virtual)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC

    # reasonable and standard
    add_compile_options_with_check(-Wall)
    add_compile_options_with_check(-Wextra)
    add_compile_options_with_check(-pedantic)
    add_compile_options_with_check(-Werror)
    #add_compile_options_with_check(-Wfatal-errors)

    add_compile_options_with_check(-Wold-style-cast)
    # helps catch hard to track down memory errors
    add_compile_options_with_check(-Wnon-virtual-dtor)
    # warn for potential performance problem casts
    add_compile_options_with_check(-Wcast-align)
    # warn if you overload (not override) a virtual function
    add_compile_options_with_check(-Woverloaded-virtual)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    message(WARNING "Using of Intel C++ is not supported, you are on your own.")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++

    # all reasonable warnings
    add_compile_options_with_check(/W4)
    # treat warnings as errors
    add_compile_options_with_check(/Wx)
    # enable warning on thread un-safe static member initialization
    add_compile_options_with_check(/W44640)
endif()

# Other flags are not added directly globally, but using a function
# so later when defining targets, it can be set per target
function(add_libtsm_compile_options target)
    # Make all files include "config.h" by default. This shouldn't cause any
    # problems and we cannot forget to include it anymore.
    target_compile_options(${target} PRIVATE
        -include ${CMAKE_BINARY_DIR}/config.h
        -pipe
        -fno-common
        -ffast-math
        -fno-strict-aliasing
        -ffunction-sections
        -fdata-sections
    )

    # Linker flags
    ## Make the linker discard all unused symbols.
    if(APPLE)
        set(LDFLAGS "-Wl,-dead_strip -Wl,-dead_strip_dylibs -Wl,-bind_at_load")
    elseif(UNIX)
        set(LDFLAGS "-Wl,--as-needed -Wl,--gc-sections -Wl,-z,relro -Wl,-z,now")
    else()
        message("Unsupported platform, you are on your own.")
    endif()

    set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${LDFLAGS}")
endfunction()
