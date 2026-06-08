# Create the user C module library
add_library(usermod_bluepad32 INTERFACE)

# Find all C files in this folder automatically
file(GLOB USERMOD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
target_sources(usermod_bluepad32 INTERFACE ${USERMOD_SOURCES})

# Include the current directory
target_include_directories(usermod_bluepad32 INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)
