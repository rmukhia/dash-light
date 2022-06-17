# For linux_host_test

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/../fft/fft_buffer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../fft/fft_amplitude.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../core/dash_light.c
        PARENT_SCOPE
        )


set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        PARENT_SCOPE
        )
