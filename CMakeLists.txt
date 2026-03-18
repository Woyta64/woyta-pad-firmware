cmake_minimum_required(VERSION 3.13)

# Initialize Pico SDK
include(pico_sdk_import.cmake)

project(woyta_pad C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# --- 1. PYTHON DETECTION ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python3")
    message(STATUS "Using local virtual environment: venv/bin/python3")
    set(PYTHON_EXEC "${CMAKE_CURRENT_SOURCE_DIR}/venv/bin/python3")
else()
    message(STATUS "Using system python")
    set(PYTHON_EXEC "python3")
endif()

# --- 2. KEYBOARD SELECTION ---
if(NOT DEFINED kb)
    set(kb "woyta-pad")
    message(STATUS "No keyboard specified, using default: ${kb}")
endif()

set(KEYBOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/keyboards/${kb})
message(STATUS "Building for keyboard: ${kb}")

# --- 3. DEFINE PATHS ---
set(GENERATED_HEADERS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(GENERATED_KEYBOARD_HEADER ${GENERATED_HEADERS_DIRECTORY}/generated_config.h)

# --- 4. GENERATE CONFIG HEADER ---
add_custom_command(
        OUTPUT ${GENERATED_KEYBOARD_HEADER}
        COMMAND ${PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_config.py
        ${KEYBOARD_DIR}/keyboard.json
        ${GENERATED_KEYBOARD_HEADER}
        DEPENDS ${KEYBOARD_DIR}/keyboard.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_config.py
        COMMENT "Generating config header..."
)

# --- 5. ADD EXTERNAL LIBRARIES ---
add_subdirectory(lib/pico-ssd1306)

# --- 6. DEFINE SOURCES ---
set(SOURCE_FILES
        core/main.c
        core/usb_descriptors.c
        core/matrix.c
        core/encoder.c
        ${GENERATED_KEYBOARD_HEADER}
        ${KEYBOARD_DIR}/keymap.c
)

# --- 7. BUILD EXECUTABLE ---
add_executable(woyta_pad ${SOURCE_FILES})

# --- 8. INCLUDE DIRECTORIES ---
target_include_directories(woyta_pad PRIVATE
        core
        ${KEYBOARD_DIR}
        ${GENERATED_HEADERS_DIRECTORY}
)

# --- 9. LINK LIBRARIES ---
target_link_libraries(woyta_pad
        pico_stdlib
        tinyusb_device
        tinyusb_board
        hardware_i2c
        pico_ssd1306
)

# --- 10. OUTPUTS ---
pico_enable_stdio_usb(woyta_pad 0)
pico_enable_stdio_uart(woyta_pad 1)
pico_add_extra_outputs(woyta_pad)
