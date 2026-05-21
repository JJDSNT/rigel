cmake_minimum_required(VERSION 3.20)

project(rigel
    VERSION 0.1.0
    DESCRIPTION "Modular chipset emulation scaffold"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RIGEL_BUILD_TESTS "Build Rigel unit tests" ON)

add_library(rigel
    src/chipset/chipset.c
    src/agnus/agnus_state.c
    src/core/rigel.c
    src/core/rigel_context.c
    src/core/rigel_timing.c
    src/mmio/rigel_mmio.c
    src/mmio/custom_regs.c
    src/irq/rigel_irq.c
    src/irq/intreq.c
    src/irq/intena.c
    src/agnus/agnus.c
    src/agnus/agnus_regs.c
    src/agnus/beam.c
    src/agnus/dma.c
    src/agnus/copper.c
    src/agnus/blitter.c
    src/agnus/blitter/blitter_command.c
    src/agnus/blitter/blitter_dma.c
    src/agnus/blitter/blitter_ref.c
    src/agnus/blitter/blitter_regs.c
    src/agnus/blitter/blitter_timing.c
    src/agnus/bitplanes.c
    src/denise/denise.c
    src/denise/denise_regs.c
    src/denise/video.c
    src/denise/sprites.c
    src/denise/color.c
    src/paula/paula.c
    src/paula/paula_state.c
    src/paula/paula_interrupts.c
    src/paula/paula_regs.c
    src/paula/audio.c
    src/paula/disk.c
    src/paula/serial.c
    src/floppy/floppy_drive.c
    src/floppy/floppy_mfm.c
    src/cia/cia.c
    src/cia/cia_timer.c
    src/cia/cia_tod.c
    src/cia/cia_ports.c
    src/bus/rigel_bus.c
    src/bus/chip_ram_if.c
    src/runtime/rigel_runtime.c
    src/runtime/single_thread.c
    src/util/log.c
    src/util/trace.c
)

target_include_directories(rigel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(rigel PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

if(RIGEL_BUILD_TESTS)
    enable_testing()

    set(RIGEL_TESTS
        test_chipset
        test_mmio
        test_irq
        test_custom_map
        test_beam
        test_cia
        test_paula
        test_blitter
        test_blitter_dma
        test_copper
    )

    foreach(test_name IN LISTS RIGEL_TESTS)
        add_executable(${test_name} tests/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE rigel)
        target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
