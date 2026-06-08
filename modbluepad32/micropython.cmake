# Create the user C module library
add_library(usermod_bluepad32 INTERFACE)

# Find all C files in this folder automatically
file(GLOB USERMOD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
target_sources(usermod_bluepad32 INTERFACE ${USERMOD_SOURCES})

# Explicitly include the Bluepad32 component directories and BTStack
target_include_directories(usermod_bluepad32 INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}
    $ENV{GITHUB_WORKSPACE}/bluepad32/src/components/bluepad32
    $ENV{GITHUB_WORKSPACE}/bluepad32/src/components/bluepad32/include
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/src
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/port/esp32/components/btstack/include
)

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)