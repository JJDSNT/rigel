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
    src/core/rigel.c
    src/core/rigel_context.c
    src/core/rigel_timing.c
    src/mmio/rigel_mmio.c
    src/mmio/custom_regs.c
    src/irq/rigel_irq.c
    src/irq/intreq.c
    src/irq/intena.c
    src/chipset/agnus/agnus_state.c
    src/chipset/agnus/agnus_regs.c
    src/chipset/agnus/beam.c
    src/chipset/agnus/dma.c
    src/chipset/agnus/copper.c
    src/chipset/agnus/blitter.c
    src/domains/beam/beam_domain.c
    src/domains/dma/dma_domain.c
    src/domains/copper/copper_domain.c
    src/domains/blitter/blitter_domain.c
    src/domains/interrupt/interrupt_domain.c
    src/domains/disk/disk_domain.c
    src/domains/serial/serial_domain.c
    src/domains/audio/audio_domain.c
    src/domains/input/input_domain.c
    src/chipset/agnus/blitter/blitter_command.c
    src/chipset/agnus/blitter/blitter_dma.c
    src/chipset/agnus/blitter/blitter_ref.c
    src/chipset/agnus/blitter/blitter_regs.c
    src/chipset/agnus/blitter/blitter_timing.c
    src/chipset/agnus/bitplanes.c
    src/chipset/denise/denise_regs.c
    src/chipset/denise/video.c
    src/chipset/denise/sprites.c
    src/chipset/denise/color.c
    src/chipset/paula/paula.c
    src/chipset/paula/paula_state.c
    src/chipset/paula/paula_interrupts.c
    src/chipset/paula/paula_regs.c
    src/chipset/paula/audio.c
    src/chipset/paula/disk.c
    src/chipset/paula/input.c
    src/chipset/paula/serial.c
    src/floppy/floppy_drive.c
    src/floppy/floppy_mfm.c
    src/cia/cia.c
    src/cia/cia_interrupt.c
    src/cia/cia_serial.c
    src/cia/cia_timers.c
    src/cia/cia_tod.c
    src/cia/cia_ports.c
    src/rtc/rtc.c
    src/bus/rigel_bus.c
    src/bus/chip_ram_if.c
    src/runtime/rigel_runtime.c
    src/runtime/single_thread.c
    src/debug/log.c
    src/debug/trace.c
)

target_include_directories(rigel
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/chipset
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
        test_agnus_domains
        test_cia
        test_floppy
        test_paula
        test_rtc
        test_serial
        test_audio
        test_input
        test_blitter
        test_blitter_dma
        test_copper
    )

    foreach(test_name IN LISTS RIGEL_TESTS)
        add_executable(${test_name} tests/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE rigel)
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/chipset
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
