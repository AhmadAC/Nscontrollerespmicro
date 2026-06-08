# Create the user C module library
add_library(usermod_bluepad32 INTERFACE)

# Find all C files in this folder automatically
file(GLOB USERMOD_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.c)
target_sources(usermod_bluepad32 INTERFACE ${USERMOD_SOURCES})

# Base Include Directories
set(BLUEPAD32_INCLUDES 
    ${CMAKE_CURRENT_LIST_DIR}
    $ENV{GITHUB_WORKSPACE}/bluepad32/src/components/bluepad32
    $ENV{GITHUB_WORKSPACE}/bluepad32/src/components/bluepad32/include
    $ENV{GITHUB_WORKSPACE}/bluepad32/src/components/bluepad32/include/platform
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/src
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/port/esp32/components/btstack/include
)

# Optional 3rd-party BTStack Includes (added only if they exist to prevent CMake errors)
set(BTSTACK_3RD_PARTY "$ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/3rd-party")

# List of all potential 3rd-party directories that btstack.h might reference
set(OPTIONAL_DIRS
    "bluedroid/encoder/include"
    "bluedroid/decoder/include"
    "micro-ecc"
    "hxcmod-player"
    "hxcmod-player/mod"
    "yxml"
    "mdns"
    "rijndael"
    "tinydir"
)

foreach(dir ${OPTIONAL_DIRS})
    if(EXISTS "${BTSTACK_3RD_PARTY}/${dir}")
        list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/${dir}")
    endif()
endforeach()

# Explicitly apply the valid include directories to the module
target_include_directories(usermod_bluepad32 INTERFACE ${BLUEPAD32_INCLUDES})

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)