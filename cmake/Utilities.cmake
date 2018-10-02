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

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

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

# Add and initialize a thirdparty submodule
#     Usage: add_submodule(<library_name> [THIRDPARTY_DIR <thirdparty_dir>] [PATCHES <patch>...])
#     Args:
#         <library_name> - The name and path of the library submodule to add
#         <thirdparty_dir> - [Optional] The root directory for thirdparty code
#         <patch> - [Optional] Additional patches to apply to the library
function(add_submodule library_name)
    set(options "")
    set(oneValueArgs "THIRDPARTY_DIR")
    set(multiValueArgs PATCHES)
    cmake_parse_arguments(ADD_SUBMODULE
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
        )

    if(EXISTS ADD_SUBMODULE_THIRDPARTY_DIR)
        set(third_party_dir ${ADD_SUBMODULE_THIRDPARTY_DIR})
    else()
        set(third_party_dir ${CMAKE_SOURCE_DIR}/thirdparty)
    endif()

    if(NOT EXISTS ${third_party_dir}/${library_name}/CMakeLists.txt)
        message(STATUS "   Initializing submodule")
        execute_process(COMMAND "git" "submodule" "update" "--init" "${third_party_dir}/${library_name}"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            RESULT_VARIABLE retcode
            )
        if(NOT "${retcode}" STREQUAL "0")
            message(FATAL_ERROR "Failed to checkout ${library_name} as submodule: ${retcode}")
        endif(NOT "${retcode}" STREQUAL "0")

        foreach(patch IN LISTS ADD_SUBMODULE_PATCHES)
            message(STATUS "   Applying patch ${patch}")
            get_filename_component(abs_patch ${patch} ABSOLUTE)
            execute_process(COMMAND "git" "apply" "${abs_patch}"
                WORKING_DIRECTORY "${third_party_dir}/${library_name}"
                RESULT_VARIABLE retcode
                )
            if(NOT "${retcode}" STREQUAL "0")
                message(FATAL_ERROR "Failed to intialize ${library_name} when applying ${abs_patch}: ${retcode}")
            endif(NOT "${retcode}" STREQUAL "0")
        endforeach(patch)
    endif(NOT EXISTS ${third_party_dir}/${library_name}/CMakeLists.txt)

    message("-- ${library_name} version: bundled")
    add_subdirectory(${third_party_dir}/${library_name})
endfunction(add_submodule)

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
