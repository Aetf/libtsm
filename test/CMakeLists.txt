#
# Tests
# These tests are included in a separate "test" target that can be run manually via `make test'
# But a more useful way to run the tests is using ctest, which runs valgrind on all tests.
# This is also the setup using in our CI.
# Note that ctest fails if _any_ leak is detected by valgrind. Thus, you
# need to have valgrind installed and libcheck running properly (without leaks)
# to make the tests succeed.

function(libtsm_add_test name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs LINK_LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${name} ${name})
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${name} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

libtsm_add_test(test_htable
    LINK_LIBRARIES
        check::check
)
target_link_object_libraries(test_htable
    PRIVATE
        shl
)

libtsm_add_test(test_symbol
    LINK_LIBRARIES
        tsm_test
        check::check
)

libtsm_add_test(test_screen
    LINK_LIBRARIES
        tsm_test
        check::check
)

libtsm_add_test(test_vte
    LINK_LIBRARIES
        tsm_test
        check::check
)

# This is only a quick sanity check that verifies the
# valgrind tests work properly.
libtsm_add_test(test_valgrind
    LINK_LIBRARIES
        check::check
)
# mark the test as will fail in memcheck
set_tests_properties(test_valgrind PROPERTIES LABELS memcheck-xfail)
