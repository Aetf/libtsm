# Add new build types for sanitizer and profiler
# Address sanitizer and undefined beheavior sanitizer
set(CMAKE_CXX_FLAGS_ASAN "-g -O1 -fno-omit-frame-pointer -fsanitize=address"
    CACHE STRING "Flags used by the C++ compiler during ASan builds." FORCE)

set(CMAKE_C_FLAGS_ASAN "${CMAKE_CXX_FLAGS_ASAN}"
    CACHE STRING "Flags used by the C compiler during ASan builds." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address"
    CACHE STRING "Flags used for linking binaries during ASan builds." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_ASAN "-fsanitize=address"
    CACHE STRING "Flags used by the shared libraries linker during ASan builds." FORCE)

# Thread sanitizer
set(CMAKE_CXX_FLAGS_TSAN "-DNDEBUG -g -O0 -fno-omit-frame-pointer -fsanitize=thread"
    CACHE STRING "Flags used by the C++ compiler during TSan builds." FORCE)

set(CMAKE_C_FLAGS_TSAN "${CMAKE_CXX_FLAGS_TSAN}"
    CACHE STRING "Flags used by the C compiler during TSan builds." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Flags used for linking binaries during TSan builds." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_TSAN "-fsanitize=thread"
    CACHE STRING "Flags used by the shared libraries linker during TSan builds." FORCE)

# Memory sanitizer
set(CMAKE_CXX_FLAGS_MSAN "-DNDEBUG -g -O1 -fno-omit-frame-pointer -fsanitize=memory"
    CACHE STRING "Flags used by the C++ compiler during MSan builds." FORCE)

set(CMAKE_C_FLAGS_MSAN "${CMAKE_CXX_FLAGS_MSAN}"
    CACHE STRING "Flags used by the C compiler during MSan builds." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_MSAN "-fsanitize=memory"
    CACHE STRING "Flags used for linking binaries during MSan builds." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_MSAN "-fsanitize=memory"
    CACHE STRING "Flags used by the shared libraries linker during MSan builds." FORCE)

set(KNOWN_BUILD_TYPES "")
foreach(build IN ITEMS ASan TSan MSan)
    string(TOUPPER ${build} build_upper)
    mark_as_advanced(
        CMAKE_CXX_FLAGS_${build_upper}
        CMAKE_C_FLAGS_${build_upper}
        CMAKE_EXE_LINKER_FLAGS_${build_upper}
        CMAKE_SHARED_LINKER_FLAGS_${build_upper}
    )
    list(APPEND KNOWN_BUILD_TYPES ${build})
endforeach()
list(APPEND KNOWN_BUILD_TYPES Debug Release RelWithDebInfo MinSizeRel)

if (NOT CMAKE_BUILD_TYPE IN_LIST KNOWN_BUILD_TYPES)
    message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}. Choices are ${KNOWN_BUILD_TYPES}")
endif()

# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
    CACHE STRING
    "Choose the type of build, options are: None;${KNOWN_BUILD_TYPES}."
    FORCE)

message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")
