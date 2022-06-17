cmake_minimum_required(VERSION 3.10)

# set the project name
project(test-app)


# --------------------------------------#

# Essential Source files
set(SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.c
        # add sources in current directory here
        ../main/fft/fft_buffer.c ../main/fft/fft_buffer.h mock/headers/freertos/portmacro.h)

set(INCLUDE_DIRS '')

# --------------------------------------#

# Unity Framework
add_subdirectory(Unity)

# --------------------------------------#

# Mock
add_subdirectory(mock)
# --------------------------------------#

# Essential Includes
# --------------------------------------#

add_subdirectory(../main/cmake_linuxtest dl_main)
# --------------------------------------#

# Add Macro For Test Compilation
add_compile_definitions(CAN5_LINUX_HOST_TEST)
add_compile_definitions(CONFIG_LOG_DEFAULT_LEVEL=1)

# Other options
add_compile_options(-Wno-format)
include_directories(${INCLUDE_DIRS})


add_executable(test-app ${SOURCE})
target_link_libraries(test-app unity m)
# Copy pretest script to build
add_custom_command(TARGET test-app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/pre_test.sh
        ${CMAKE_CURRENT_BINARY_DIR}/pre_test.sh)
