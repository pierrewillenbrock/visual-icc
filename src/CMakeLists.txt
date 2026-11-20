qt_add_resources(RESOURCES resources.qrc)

qt_add_executable(visual-icc
    main.cpp
    visual-icc.ui
    visual-icc.cpp
    coloradjustingwidget.cpp
    findcolorprofile.cpp
    colorspaceinformationdisplay.cpp
    colorspaceinfographicswidget.cpp
    extract_colorspace_transfer_tables.cpp
    colorspacetransferfunctioneditor.cpp
    ${RESOURCES}
)

# Use the Widgets module from Qt 6.
target_link_libraries(visual-icc PRIVATE Qt6::Widgets Qt6::DBus)
#target_link_libraries(visual-icc PRIVATE PkgConfig::LCMS)
target_include_directories(visual-icc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(visual-icc PRIVATE cxx_std_20)
target_compile_options(visual-icc PRIVATE -Wall -O2)
set_target_properties(visual-icc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Install the executable
install(TARGETS visual-icc DESTINATION bin)



add_subdirectory(colorprofileproviders)
