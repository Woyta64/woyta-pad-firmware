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

# --- 3. READ CONFIGURATION (OLED CHECK) ---
file(READ "${KEYBOARD_DIR}/keyboard.json" JSON_CONTENT)

# Extract and sanitize OLED enabled flag
string(JSON OLED_ENABLED_RAW ERROR_VARIABLE JSON_ERR GET "${JSON_CONTENT}" "oled" "enabled")
string(STRIP "${OLED_ENABLED_RAW}" OLED_ENABLED_CLEAN)
string(TOUPPER "${OLED_ENABLED_CLEAN}" OLED_ENABLED_UPPER)

if(OLED_ENABLED_UPPER STREQUAL "TRUE" OR
        OLED_ENABLED_UPPER STREQUAL "ON"   OR
        OLED_ENABLED_UPPER STREQUAL "1")
    set(IS_OLED_ON TRUE)
else()
    set(IS_OLED_ON FALSE)
endif()

message(STATUS "OLED Configuration: ${IS_OLED_ON}")

# --- 4. DEFINE PATHS ---
set(GENERATED_HEADERS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(GENERATED_KEYBOARD_HEADER ${GENERATED_HEADERS_DIRECTORY}/generated_config.h)

# --- 5. GENERATE CONFIG HEADER ---
add_custom_command(
        OUTPUT ${GENERATED_KEYBOARD_HEADER}
        COMMAND ${PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_config.py
        ${KEYBOARD_DIR}/keyboard.json
        ${GENERATED_KEYBOARD_HEADER}
        DEPENDS ${KEYBOARD_DIR}/keyboard.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_config.py
        COMMENT "Generating config header..."
)

# --- 6. ADD EXTERNAL LIBRARIES ---
# REMOVED: add_subdirectory(lib/pico-ssd1306) -- We compile source directly now

# --- 7. DEFINE SOURCES ---
set(SOURCE_FILES
        core/main.c
        core/usb_descriptors.c
        core/matrix.c
        core/encoder.c
        ${GENERATED_KEYBOARD_HEADER}
        ${KEYBOARD_DIR}/keymap.c
)

# --- 8. OLED ASSET GENERATION ---
if(IS_OLED_ON)
    set(ASSETS_DIR ${KEYBOARD_DIR}/assets)
    set(GENERATED_ASSETS_HEADER ${GENERATED_HEADERS_DIRECTORY}/generated_bitmaps.h)

    # Find all image types
    file(GLOB IMAGE_ASSETS
            "${ASSETS_DIR}/*.png"
            "${ASSETS_DIR}/*.jpg"
            "${ASSETS_DIR}/*.jpeg"
            "${ASSETS_DIR}/*.bmp"
    )

    # Rule to generate bitmaps header
    add_custom_command(
            OUTPUT ${GENERATED_ASSETS_HEADER}
            COMMAND ${PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/tools/image_to_byte_array.py
            ${ASSETS_DIR}
            ${GENERATED_ASSETS_HEADER}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/image_to_byte_array.py
            ${IMAGE_ASSETS}
            COMMENT "Converting images to C header..."
    )

    # Add OLED files, Header, AND THE DRIVER SOURCE directly
    list(APPEND SOURCE_FILES
            core/oled.c
            ${GENERATED_ASSETS_HEADER}
            ${KEYBOARD_DIR}/assets.h
            lib/pico-ssd1306/ssd1306.c  # <--- Driver Source added here
    )
endif()

# --- 9. BUILD EXECUTABLE ---
add_executable(woyta_pad ${SOURCE_FILES})

# --- 10. INCLUDE DIRECTORIES ---
target_include_directories(woyta_pad PRIVATE
        core
        ${KEYBOARD_DIR}
        ${GENERATED_HEADERS_DIRECTORY}
        lib/pico-ssd1306 # <--- So we can find ssd1306.h and font.h
)

# --- 11. LINK LIBRARIES ---
set(LIBS
        pico_stdlib
        tinyusb_device
        tinyusb_board
)

if(IS_OLED_ON)
    list(APPEND LIBS
            hardware_i2c
            # No pico-ssd1306 link needed; source is included above
    )
endif()

target_link_libraries(woyta_pad ${LIBS})

# --- 12. OUTPUTS ---
pico_enable_stdio_usb(woyta_pad 0)
pico_enable_stdio_uart(woyta_pad 1)
pico_add_extra_outputs(woyta_pad)
