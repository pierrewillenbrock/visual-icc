qt_add_executable(visual-icc
    main.cpp
    visual-icc.ui
    visual-icc.cpp
)

# Use the Widgets module from Qt 6.
target_link_libraries(visual-icc PRIVATE Qt6::Widgets)

# Install the executable
install(TARGETS visual-icc DESTINATION bin)
