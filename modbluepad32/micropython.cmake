# Create an INTERFACE library for our C module
add_library(usermod_bluepad32 INTERFACE)

# Add our source files
target_sources(usermod_bluepad32 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/bluepad32_mpy.c
)

# Add the current directory as an include directory
target_include_directories(usermod_bluepad32 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link our library to the usermod target
target_link_libraries(usermod_bluepad32 INTERFACE usermod)
