include(CheckCCompilerFlag)

#
# Check compiler flag support with cached support.
# Shadows the internal check_c_compiler_flag
#
macro(libtsm_check_c_compiler_flag flag retvar)
    string(REPLACE "-" "_" cache_name "COMPILER_SUPPORT_${flag}")
    check_c_compiler_flag(${flag} ${cache_name})
    set(${retvar} ${cache_name})
endmacro()

# Add a global default compile option
function(add_compile_options_with_check flag)
    libtsm_check_c_compiler_flag(${flag} ret)
    if(${ret})
        add_compile_options(${flag})
    endif()
endfunction()

# Add a new private compile option to a target
function(target_compile_options_with_check target flag)
    libtsm_check_c_compiler_flag(${flag} ret)
    if(${ret})
        target_compile_options(${target} PRIVATE ${flag})
    endif()
endfunction()

# Add compile option to a build type with check
function(build_type_add_compile_option_with_check buildtype flag)
    libtsm_check_c_compiler_flag(${flag} ret)
    if(${ret})
        set(CMAKE_CXX_FLAGS_${buildtype} "${CMAKE_CXX_FLAGS_${buildtype}}" ${flag})
    endif()
endfunction()

# Print all properties for a target
# From https://stackoverflow.com/questions/32183975/how-to-print-all-the-properties-of-a-target-in-cmake
#
# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
# Convert command output into a CMake list
string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message("There is no target named '${tgt}'")
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
    if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
        continue()
    endif()
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message ("${tgt} ${prop} = ${propval}")
        endif()
    endforeach(prop)
endfunction(print_target_properties)

# A simpler version to parse package components
# Copied from extra-cmake-modules
macro(find_package_parse_components module_name)
    set(fppc_options)
    set(fppc_oneValueArgs RESULT_VAR)
    set(fppc_multiValueArgs KNOWN_COMPONENTS DEFAULT_COMPONENTS)
    cmake_parse_arguments(FPPC "${fppc_options}" "${fppc_oneValueArgs}" "${fppc_multiValueArgs}" ${ARGN})

    if(FPPC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to find_package_parse_components: ${FPPC_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT FPPC_RESULT_VAR)
        message(FATAL_ERROR "Missing RESULT_VAR argument to find_package_parse_components")
    endif()
    if(NOT FPPC_KNOWN_COMPONENTS)
        message(FATAL_ERROR "Missing KNOWN_COMPONENTS argument to find_package_parse_components")
    endif()
    if(NOT FPPC_DEFAULT_COMPONENTS)
        set(FPPC_DEFAULT_COMPONENTS ${FPPC_KNOWN_COMPONENTS})
    endif()

    if(${module_name}_FIND_COMPONENTS)
        set(fppc_requestedComps ${${module_name}_FIND_COMPONENTS})

        list(REMOVE_DUPLICATES fppc_requestedComps)

        # This makes sure components are listed in the same order as
        # KNOWN_COMPONENTS (potentially important for inter-dependencies)
        set(${FPPC_RESULT_VAR})
        foreach(fppc_comp ${FPPC_KNOWN_COMPONENTS})
            list(FIND fppc_requestedComps "${fppc_comp}" fppc_index)
            if(NOT "${fppc_index}" STREQUAL "-1")
                list(APPEND ${FPPC_RESULT_VAR} "${fppc_comp}")
                list(REMOVE_AT fppc_requestedComps ${fppc_index})
            endif()
        endforeach()
        # if there are any left, they are unknown components
        if(fppc_requestedComps)
            set(fppc_msgType STATUS)
            if(${module_name}_FIND_REQUIRED)
                set(fppc_msgType FATAL_ERROR)
            endif()
            if(NOT ${module_name}_FIND_QUIETLY)
                message(${fppc_msgType} "${module_name}: requested unknown components ${fppc_requestedComps}")
            endif()
            return()
        endif()
    else()
        set(${FPPC_RESULT_VAR} ${FPPC_DEFAULT_COMPONENTS})
    endif()
endmacro()

#
# Creates an imported library target for each component. See ECM's documentation for usage.
#
# This version addtionally treats empty <name>_<component>_lib variable as an indication for interface only library
#
# The following additionall keyword arguments are accepted:
#   NAMESPACE     - The namespace to use for the imported target, default to <module_name>
#
macro(find_package_handle_library_components module_name)
    set(fphlc_options SKIP_PKG_CONFIG SKIP_DEPENDENCY_HANDLING)
    set(fphlc_oneValueArgs NAMESPACE)
    set(fphlc_multiValueArgs COMPONENTS)
    cmake_parse_arguments(FPHLC "${fphlc_options}" "${fphlc_oneValueArgs}" "${fphlc_multiValueArgs}" ${ARGN})

    if(FPHLC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to find_package_handle_components: ${FPHLC_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT FPHLC_COMPONENTS)
        message(FATAL_ERROR "Missing COMPONENTS argument to find_package_handle_components")
    endif()

    if(NOT FPHLC_NAMESPACE)
        set(FPHLC_NAMESPACE ${module_name})
    endif()

    include(FindPackageHandleStandardArgs)
    find_package(PkgConfig QUIET)
    foreach(fphlc_comp ${FPHLC_COMPONENTS})
        set(fphlc_dep_vars)
        set(fphlc_dep_targets)
        if(NOT SKIP_DEPENDENCY_HANDLING)
            foreach(fphlc_dep ${${module_name}_${fphlc_comp}_component_deps})
                list(APPEND fphlc_dep_vars "${module_name}_${fphlc_dep}_FOUND")
                list(APPEND fphlc_dep_targets "${FPHLC_NAMESPACE}::${fphlc_dep}")
            endforeach()
        endif()

        if(NOT FPHLC_SKIP_PKG_CONFIG AND ${module_name}_${fphlc_comp}_pkg_config)
            pkg_check_modules(PKG_${module_name}_${fphlc_comp} QUIET
                ${${module_name}_${fphlc_comp}_pkg_config})
        endif()

        if("${${module_name}_${fphlc_comp}_lib}" STREQUAL "")
            set(fphlc_library_type INTERFACE)
            set(fphlc_required_vars "${module_name}_${fphlc_comp}_INCLUDE_DIR")
        else()
            set(fphlc_library_type UNKNOWN)
            set(fphlc_required_vars "${module_name}_${fphlc_comp}_LIBRARY;${module_name}_${fphlc_comp}_INCLUDE_DIR")
        endif()

        find_path(${module_name}_${fphlc_comp}_INCLUDE_DIR
            NAMES ${${module_name}_${fphlc_comp}_header}
            HINTS ${PKG_${module_name}_${fphlc_comp}_INCLUDE_DIRS}
            PATH_SUFFIXES ${${module_name}_${fphlc_comp}_header_subdir}
            )
        find_library(${module_name}_${fphlc_comp}_LIBRARY
            NAMES ${${module_name}_${fphlc_comp}_lib}
            HINTS ${PKG_${module_name}_${fphlc_comp}_LIBRARY_DIRS}
            )

        set(${module_name}_${fphlc_comp}_VERSION "${PKG_${module_name}_${fphlc_comp}_VERSION}")
        if(NOT ${module_name}_VERSION)
            set(${module_name}_VERSION ${${module_name}_${fphlc_comp}_VERSION})
        endif()

        find_package_handle_standard_args(${module_name}_${fphlc_comp}
            FOUND_VAR
            ${module_name}_${fphlc_comp}_FOUND
            REQUIRED_VARS
            ${fphlc_required_vars}
            ${fphlc_dep_vars}
            VERSION_VAR
            ${module_name}_${fphlc_comp}_VERSION
        )

        mark_as_advanced(
            ${module_name}_${fphlc_comp}_LIBRARY
            ${module_name}_${fphlc_comp}_INCLUDE_DIR
        )

        if(${module_name}_${fphlc_comp}_FOUND)
            if("${fphlc_library_type}" STREQUAL "UNKNOWN")
                list(APPEND ${module_name}_LIBRARIES
                    "${${module_name}_${fphlc_comp}_LIBRARY}")
            endif()
            list(APPEND ${module_name}_INCLUDE_DIRS
                "${${module_name}_${fphlc_comp}_INCLUDE_DIR}")
            set(${module_name}_DEFINITIONS
                ${${module_name}_DEFINITIONS}
                ${PKG_${module_name}_${fphlc_comp}_DEFINITIONS})
            if(NOT TARGET ${FPHLC_NAMESPACE}::${fphlc_comp})
                add_library(${FPHLC_NAMESPACE}::${fphlc_comp} ${fphlc_library_type} IMPORTED)
                set_target_properties(${FPHLC_NAMESPACE}::${fphlc_comp} PROPERTIES
                    INTERFACE_COMPILE_OPTIONS "${PKG_${module_name}_${fphlc_comp}_DEFINITIONS}"
                    INTERFACE_INCLUDE_DIRECTORIES "${${module_name}_${fphlc_comp}_INCLUDE_DIR}"
                    INTERFACE_LINK_LIBRARIES "${fphlc_dep_targets}"
                )
                if("${fphlc_library_type}" STREQUAL "UNKNOWN")
                    set_target_properties(${FPHLC_NAMESPACE}::${fphlc_comp} PROPERTIES
                        IMPORTED_LOCATION "${${module_name}_${fphlc_comp}_LIBRARY}"
                    )
                endif()
            endif()
            list(APPEND ${module_name}_TARGETS
                "${FPHLC_NAMESPACE}::${fphlc_comp}")
        endif()
    endforeach()
    if(${module_name}_LIBRARIES)
        list(REMOVE_DUPLICATES ${module_name}_LIBRARIES)
    endif()
    if(${module_name}_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES ${module_name}_INCLUDE_DIRS)
    endif()
    if(${module_name}_DEFINITIONS)
        list(REMOVE_DUPLICATES ${module_name}_DEFINITIONS)
    endif()
    if(${module_name}_TARGETS)
        list(REMOVE_DUPLICATES ${module_name}_TARGETS)
    endif()
endmacro()

#
# Helper to link to an object library.
#
# The advantage of using this over target_link_library directly
# is that when used privately, it will not pollute target exports,
# See: https://gitlab.kitware.com/cmake/cmake/issues/17357
#
# This is done by not involving target_link_libraries but directly set
# properties.
#
function(target_link_object_libraries target)
    set(tlol_options)
    set(tlol_oneValueArgs)
    set(tlol_multiValueArgs PRIVATE INTERFACE PUBLIC)
    cmake_parse_arguments(TLOL "${tlol_options}" "${tlol_oneValueArgs}" "${tlol_multiValueArgs}" ${ARGN})

    if(TLOL_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments to target_link_object_libraries: ${TLOL_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT TLOL_PRIVATE AND NOT TLOL_INTERFACE AND NOT TLOL_PUBLIC)
        message(FATAL_ERROR "Missing PRIVATE|INTERFACE|PUBLIC argument to target_link_object_libraries")
    endif()

    foreach(lib IN LISTS TLOL_PRIVATE)
        target_sources(${target} PRIVATE $<TARGET_OBJECTS:${lib}> $<TARGET_PROPERTY:${lib},INTERFACE_SOURCES>)
        target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
        target_compile_options(${target} PRIVATE $<TARGET_PROPERTY:${lib},INTERFACE_COMPILE_OPTIONS>)
        # This is not supported, as it will make exporting ${target} also needs exporting ${lib} even in PRIVATE mode
        #target_link_libraries(${target} PRIVATE $<TARGET_PROPERTY:${lib},INTERFACE_LINK_LIBRARIES>)
    endforeach()
    foreach(lib IN LISTS TLOL_INTERFACE)
        target_sources(${target} INTERFACE $<TARGET_OBJECTS:${lib}> $<TARGET_PROPERTY:${lib},INTERFACE_SOURCES>)
        target_include_directories(${target} INTERFACE $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
        target_compile_options(${target} INTERFACE $<TARGET_PROPERTY:${lib},INTERFACE_COMPILE_OPTIONS>)
        target_link_libraries(${target} INTERFACE $<TARGET_PROPERTY:${lib},INTERFACE_LINK_LIBRARIES>)
    endforeach()
    foreach(lib IN LISTS TLOL_PUBLIC)
        target_sources(${target} PUBLIC $<TARGET_OBJECTS:${lib}> $<TARGET_PROPERTY:${lib},INTERFACE_SOURCES>)
        target_include_directories(${target} PUBLIC $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
        target_compile_options(${target} PUBLIC $<TARGET_PROPERTY:${lib},INTERFACE_COMPILE_OPTIONS>)
    endforeach()
endfunction()
