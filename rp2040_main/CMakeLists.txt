cmake_minimum_required(VERSION 3.13...3.27)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(rp2040_main)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE INCLUDE_SOURCES "${CMAKE_SOURCE_DIR}/include/LoRa-RP2040.cpp")
# file(GLOB_RECURSE INCLUDE_SOURCES "${CMAKE_SOURCE_DIR}/include/*.c")

message(${CMAKE_SOURCE_DIR})
message(STATUS ${INCLUDE_SOURCES})

# Define your main source file
set(MAIN_SOURCE "${CMAKE_SOURCE_DIR}/src/rp2040_main.cpp")
set(LORA_SOURCE "${CMAKE_SOURCE_DIR}/include/LoRa-RP2040.cpp")
set(LORA1_SOURCE "${CMAKE_SOURCE_DIR}/include/Print.cpp")

# Create executable using main + include sources
add_executable(${PROJECT_NAME} ${MAIN_SOURCE} ${LORA_SOURCE} ${LORA1_SOURCE})

# Link with the Pico SDK libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_gpio
    hardware_adc
    hardware_spi
    # add others as needed (hardware_uart, hardware_pwm, etc.)
)

# Add include directory for headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Enable USB and UART stdio (optional)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create UF2, BIN, and HEX files
pico_add_extra_outputs(${PROJECT_NAME})