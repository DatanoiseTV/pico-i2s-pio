cmake_minimum_required(VERSION 3.12)
project(pico-i2s-pio)

add_library(pico-i2s-pio STATIC i2s.c)
pico_generate_pio_header(pico-i2s-pio ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)
target_link_libraries(pico-i2s-pio
        pico_stdlib
        pico_multicore
        hardware_dma
        hardware_pio
        hardware_clocks
        hardware_sync
        )

target_include_directories(pico-i2s-pio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})