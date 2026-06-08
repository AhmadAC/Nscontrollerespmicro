# Create the user C module library
add_library(usermod_bluepad32 INTERFACE)

# Find all C files in this folder automatically
file(GLOB USERMOD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
target_sources(usermod_bluepad32 INTERFACE ${USERMOD_SOURCES})

# Explicitly include the Bluepad32 AND BTStack header directories
target_include_directories(usermod_bluepad32 INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}
    $ENV{EXTRA_COMPONENT_DIRS}/bluepad32/include
    $ENV{EXTRA_COMPONENT_DIRS}/btstack/src
    $ENV{EXTRA_COMPONENT_DIRS}/btstack/platform/embedded
    $ENV{EXTRA_COMPONENT_DIRS}/btstack/platform/freertos
    $ENV{EXTRA_COMPONENT_DIRS}/btstack/port/esp32
)

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)
