#
# GtkTsm - Example
#
add_executable(gtktsm
    gtktsm.c
    gtktsm-app.c
    gtktsm-terminal.c
    gtktsm-win.c
)
target_link_libraries(gtktsm
    PRIVATE
        tsm

        m
        PkgConfig::GTK3
        PkgConfig::Cairo
        PkgConfig::Pango
        PkgConfig::PangoCairo
        XKB::XKBCommon
)
target_link_object_libraries(gtktsm
    PRIVATE
        shl
)
add_libtsm_compile_options(gtktsm)

install(TARGETS gtktsm
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
