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
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/src
    $ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/port/esp32/components/btstack/include
)

# Optional 3rd-party BTStack Includes (added only if they exist to prevent CMake errors)
set(BTSTACK_3RD_PARTY "$ENV{GITHUB_WORKSPACE}/bluepad32/external/btstack/3rd-party")

if(EXISTS "${BTSTACK_3RD_PARTY}/bluedroid/encoder/include")
    list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/bluedroid/encoder/include")
endif()

if(EXISTS "${BTSTACK_3RD_PARTY}/bluedroid/decoder/include")
    list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/bluedroid/decoder/include")
endif()

if(EXISTS "${BTSTACK_3RD_PARTY}/micro-ecc")
    list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/micro-ecc")
endif()

if(EXISTS "${BTSTACK_3RD_PARTY}/hxcmod-player")
    list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/hxcmod-player")
endif()

if(EXISTS "${BTSTACK_3RD_PARTY}/hxcmod-player/mod")
    list(APPEND BLUEPAD32_INCLUDES "${BTSTACK_3RD_PARTY}/hxcmod-player/mod")
endif()

# Explicitly apply the valid include directories to the module
target_include_directories(usermod_bluepad32 INTERFACE ${BLUEPAD32_INCLUDES})

# Link the user module to MicroPython
target_link_libraries(usermod INTERFACE usermod_bluepad32)