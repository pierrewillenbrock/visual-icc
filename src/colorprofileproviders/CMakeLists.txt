

# this only builds the given plugin if the build requirements are present.
# at the same time, the given plugin can only be loaded if its
# shared object requirements are present.

find_package(KF6Screen QUIET)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT QUIET IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS QUIET wayland-protocols)
pkg_get_variable(WP_DIR wayland-protocols pkgdatadir)
find_program(WAYLAND_SCANNER wayland-scanner QUIET)
pkg_check_modules(XCB QUIET IMPORTED_TARGET xcb xcb-randr)

if(KF6Screen_FOUND)
    qt_add_plugin(cppp_kde CLASS_NAME KdeColorProfileProvider)
    set_target_properties(cppp_kde PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/colorprofile)
    target_sources(cppp_kde PRIVATE kde.cpp)
    target_link_libraries(cppp_kde PRIVATE Qt6::Core Qt6::Gui KF6::Screen)
    target_compile_features(cppp_kde PRIVATE cxx_std_20)
    target_compile_options(cppp_kde PRIVATE -Wall -O2)

    install(TARGETS cppp_kde
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
    )
endif()


if(WAYLAND_SCANNER AND WAYLAND_PROTOCOLS_FOUND AND WAYLAND_CLIENT_FOUND)
    set(CM_XML ${WP_DIR}/staging/color-management/color-management-v1.xml)
    set(GEN ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(
        OUTPUT ${GEN}/color-management-v1-client-protocol.h ${GEN}/color-management-v1-protocol.c
        COMMAND ${WAYLAND_SCANNER} client-header ${CM_XML} ${GEN}/color-management-v1-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} private-code  ${CM_XML} ${GEN}/color-management-v1-protocol.c
        DEPENDS ${CM_XML})

    qt_add_plugin(cppp_wayland CLASS_NAME WaylandColorProvider)
    set_target_properties(cppp_wayland PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/colorprofile)
    target_sources(cppp_wayland PRIVATE
        wayland.cpp
        ${GEN}/color-management-v1-protocol.c
        )
    target_include_directories(cppp_wayland PRIVATE ${GEN})
    target_link_libraries(cppp_wayland PRIVATE Qt6::Core Qt6::Gui PkgConfig::WAYLAND_CLIENT)
    target_compile_features(cppp_wayland PRIVATE cxx_std_20)
    target_compile_options(cppp_wayland PRIVATE -Wall -O2)

    install(TARGETS cppp_wayland
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
    )
endif()

if(XCB_FOUND)
    qt_add_plugin(cppp_x11 CLASS_NAME X11ColorProvider)
    set_target_properties(cppp_x11 PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/colorprofile)
    target_sources(cppp_x11 PRIVATE x11.cpp)
    target_link_libraries(cppp_x11 PRIVATE Qt6::Core Qt6::Gui PkgConfig::XCB)
    target_compile_features(cppp_x11 PRIVATE cxx_std_20)
    target_compile_options(cppp_x11 PRIVATE -Wall -O2)

    install(TARGETS cppp_x11
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
    )
endif()

qt_add_plugin(cppp_gnome CLASS_NAME GnomeColorProvider)
set_target_properties(cppp_gnome PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/colorprofile)
target_sources(cppp_gnome PRIVATE gnome.cpp)
target_link_libraries(cppp_gnome PRIVATE Qt6::Core Qt6::Gui Qt6::DBus)
target_compile_features(cppp_gnome PRIVATE cxx_std_20)
target_compile_options(cppp_gnome PRIVATE -Wall -O2)

install(TARGETS cppp_gnome
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/colorprofile
)
