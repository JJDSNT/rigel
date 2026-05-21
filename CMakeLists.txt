cmake_minimum_required(VERSION 3.20)

project(riegel
    VERSION 0.1.0
    DESCRIPTION "Modular chipset emulation scaffold"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RIEGEL_BUILD_TESTS "Build Riegel unit tests" ON)

add_library(riegel
    src/chipset/chipset.c
    src/agnus/agnus_state.c
    src/core/riegel.c
    src/core/riegel_context.c
    src/core/riegel_timing.c
    src/mmio/riegel_mmio.c
    src/mmio/custom_regs.c
    src/irq/riegel_irq.c
    src/irq/intreq.c
    src/irq/intena.c
    src/agnus/agnus.c
    src/agnus/agnus_regs.c
    src/agnus/beam.c
    src/agnus/dma.c
    src/agnus/copper.c
    src/agnus/blitter.c
    src/agnus/blitter/blitter_command.c
    src/agnus/blitter/blitter_regs.c
    src/agnus/blitter/blitter_timing.c
    src/agnus/bitplanes.c
    src/denise/denise.c
    src/denise/denise_regs.c
    src/denise/video.c
    src/denise/sprites.c
    src/denise/color.c
    src/paula/paula.c
    src/paula/paula_regs.c
    src/paula/audio.c
    src/paula/disk.c
    src/paula/serial.c
    src/cia/cia.c
    src/cia/cia_timer.c
    src/cia/cia_tod.c
    src/cia/cia_ports.c
    src/bus/riegel_bus.c
    src/bus/chip_ram_if.c
    src/runtime/riegel_runtime.c
    src/runtime/single_thread.c
    src/util/log.c
    src/util/trace.c
)

target_include_directories(riegel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(riegel PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

if(RIEGEL_BUILD_TESTS)
    enable_testing()

    set(RIEGEL_TESTS
        test_chipset
        test_mmio
        test_irq
        test_custom_map
        test_beam
        test_cia
        test_blitter
        test_copper
    )

    foreach(test_name IN LISTS RIEGEL_TESTS)
        add_executable(${test_name} tests/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE riegel)
        target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
