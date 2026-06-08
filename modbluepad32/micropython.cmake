# Create the user C module library
add_library(usermod_bluepad32 INTERFACE)

# Find all C files in this folder automatically
file(GLOB USERMOD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
target_sources(usermod_bluepad32 INTERFACE ${USERMOD_SOURCES})

# Explicitly include the local directory AND the Bluepad32 header directories
# so the compiler can find "uni.h" and the BTStack dependencies
target_include_directories(usermod_bluepad32 INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}
    $ENV{EXTRA_COMPONENT_DIRS}/bluepad32/include
    $ENV{EXTRA_COMPONENT_DIRS}/btstack/include
)

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)
