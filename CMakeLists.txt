cmake_minimum_required(VERSION 3.20)

project(rigel
    VERSION 0.1.0
    DESCRIPTION "Modular chipset emulation scaffold"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RIGEL_BUILD_TESTS    "Build Rigel unit tests"          ON)
option(RIGEL_BUILD_HARNESS  "Build Musashi integration harness" OFF)
option(RIGEL_ENABLE_STDIO_LOG "Use stderr for Rigel logs when no host log callback is installed" ON)
option(RIGEL_ENABLE_STDLIB_ENV "Use getenv() for runtime trace knobs; disable for bare-metal targets" ON)
option(RIGEL_ENABLE_SIMD "Enable optional SIMD backends for internal pixel/buffer operations" ON)

if(RIGEL_BUILD_TESTS OR RIGEL_BUILD_HARNESS)
    enable_testing()
endif()

add_library(rigel
    src/chipset/chipset.c
    src/core/rigel.c
    src/core/rigel_beam_api.c
    src/core/rigel_context.c
    src/core/rigel_audio_api.c
    src/core/rigel_cia_api.c
    src/core/rigel_denise_api.c
    src/core/rigel_serial_api.c
    src/core/rigel_timing.c
    src/mmio/rigel_mmio.c
    src/mmio/custom_regs.c
    src/irq/rigel_irq.c
    src/irq/intreq.c
    src/irq/intena.c
    src/chipset/agnus/agnus.c
    src/chipset/agnus/beam.c
    src/chipset/agnus/dma.c
    src/chipset/agnus/copper/copper.c
    src/chipset/agnus/copper/copper_regs.c
    src/chipset/agnus/copper/copper_service.c
    src/chipset/agnus/mmio/agnus_mmio.c
    src/chipset/agnus/mmio/agnus_read.c
    src/chipset/agnus/mmio/agnus_write.c
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
    src/chipset/agnus/blitter/blitter_line.c
    src/chipset/agnus/blitter/blitter_ref.c
    src/chipset/agnus/blitter/blitter_regs.c
    src/chipset/agnus/blitter/blitter_timing.c
    src/chipset/agnus/timing/vblank.c
    src/chipset/agnus/timing/deadline.c
    src/chipset/agnus/timing/raster.c
    src/chipset/agnus/timing/slot_scheduler.c
    src/chipset/agnus/irq/agnus_irq.c
    src/chipset/agnus/bitplanes/bitplanes.c
    src/chipset/agnus/bitplanes/bitplane_fetch.c
    src/chipset/agnus/bitplanes/bitplane_pointers.c
    src/chipset/agnus/dma/refresh_dma.c
    src/chipset/agnus/dma/sprite_dma.c
    src/chipset/agnus/copper/copper_exec.c
    src/chipset/agnus/copper/copper_wait.c
    src/chipset/denise/denise.c
    src/chipset/denise/registers/denise_registers.c
    src/chipset/denise/render/compositor.c
    src/chipset/denise/render/dualpf.c
    src/chipset/denise/render/ehb.c
    src/chipset/denise/render/ham.c
    src/chipset/denise/render/playfield.c
    src/chipset/denise/render/priority.c
    src/chipset/denise/output/planar.c
    src/chipset/denise/sprites/collisions.c
    src/chipset/denise/sprites/sprite_attach.c
    src/chipset/denise/sprites/sprites_core.c
    src/chipset/denise/sprites/sprites.c
    src/chipset/denise/palette/palette.c
    src/chipset/denise/palette/color_regs.c
    src/chipset/denise/video/display_window.c
    src/chipset/denise/output/framebuffer.c
    src/chipset/denise/debug/denise_trace.c
    src/chipset/paula/paula.c
    src/chipset/paula/paula_state.c
    src/chipset/paula/paula_interrupts.c
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
    src/simd/rigel_simd.c
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

if(RIGEL_ENABLE_STDIO_LOG)
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_STDIO_LOG=1)
else()
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_STDIO_LOG=0)
endif()

if(RIGEL_ENABLE_STDLIB_ENV)
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_STDLIB_ENV=1)
else()
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_STDLIB_ENV=0)
endif()

if(RIGEL_ENABLE_SIMD)
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_SIMD=1)
else()
    target_compile_definitions(rigel PRIVATE RIGEL_ENABLE_SIMD=0)
endif()

if(RIGEL_BUILD_TESTS)
    set(RIGEL_TESTS
        test_chipset
        test_mmio
        test_irq
        test_custom_map
        test_timing
        test_beam
        test_agnus_domains
        test_cia
        test_mfm_decode
        test_floppy
        test_paula
        test_rtc
        test_serial
        test_audio
        test_input
        test_denise
        test_video_modes
        test_simd
        test_baremetal_log
        test_blitter
        test_blitter_dma
        test_blitter_channel_cost
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

    set(DENISE_UNIT_TESTS
        test_priority
        test_ham
        test_dualpf
        test_sprites
    )

    foreach(test_name IN LISTS DENISE_UNIT_TESTS)
        add_executable(${test_name}
            src/chipset/denise/tests/${test_name}.c)
        target_link_libraries(${test_name} PRIVATE rigel)
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/chipset
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    if(NOT RIGEL_ENABLE_STDIO_LOG AND CMAKE_NM)
        add_test(
            NAME test_baremetal_no_stdio
            COMMAND ${CMAKE_COMMAND}
                -DNM=${CMAKE_NM}
                -DLIB=$<TARGET_FILE:rigel>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_no_stdio.cmake
        )
    endif()
endif()

if(RIGEL_BUILD_HARNESS)
    set(MUSASHI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/musashi)

    if(NOT EXISTS ${MUSASHI_DIR}/m68kcpu.c)
        message(FATAL_ERROR
            "external/musashi is empty. Run:\n"
            "    git submodule update --init external/musashi\n"
            "    scripts/apply_musashi_patches.sh")
    endif()

    # Rigel's local Musashi changes live in patches/musashi and are applied to
    # the submodule, not committed into it. Building against a pristine tree
    # would silently lose the FPU fixes and the F-line diagnostic.
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/apply_musashi_patches.sh --check
        RESULT_VARIABLE MUSASHI_PATCHED
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT MUSASHI_PATCHED EQUAL 0)
        message(FATAL_ERROR
            "external/musashi is missing Rigel's patches. Run:\n"
            "    scripts/apply_musashi_patches.sh\n"
            "See patches/musashi/README.md.")
    endif()

    # Generate m68kops.c / m68kops.h from m68k_in.c
    add_executable(m68kmake ${MUSASHI_DIR}/m68kmake.c)
    set(MUSASHI_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/musashi_gen)
    file(MAKE_DIRECTORY ${MUSASHI_GEN_DIR})
    add_custom_command(
        OUTPUT  ${MUSASHI_GEN_DIR}/m68kops.c ${MUSASHI_GEN_DIR}/m68kops.h
        COMMAND m68kmake ${MUSASHI_GEN_DIR} ${MUSASHI_DIR}/m68k_in.c
        DEPENDS m68kmake ${MUSASHI_DIR}/m68k_in.c
    )

    add_library(musashi STATIC
        ${MUSASHI_DIR}/m68kcpu.c
        ${MUSASHI_DIR}/m68kdasm.c
        ${MUSASHI_DIR}/m68kfpu.c
        ${MUSASHI_DIR}/softfloat/softfloat.c
        ${MUSASHI_GEN_DIR}/m68kops.c
    )
    target_include_directories(musashi PUBLIC
        ${MUSASHI_DIR}
        ${MUSASHI_GEN_DIR}
    )
    target_compile_options(musashi PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-Wno-unused-parameter -Wno-sign-compare>
    )
    target_link_libraries(musashi PUBLIC m)

    add_library(rigel_harness STATIC
        harness/harness.c
        harness/hunk.c
        harness/expansions/lide.c
        harness/expansions/fastram.c
        harness/expansions/zorro.c
        harness/expansions/ata_ide.c
        harness/expansions/atapi_cdrom.c
    )
    target_link_libraries(rigel_harness PUBLIC rigel musashi)
    target_include_directories(rigel_harness PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/harness
        ${CMAKE_CURRENT_SOURCE_DIR}/harness/expansions
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MUSASHI_DIR}
    )

    set(HARNESS_TESTS
        test_blitter_stall
        test_blitter_timing
        test_endian
    )

    foreach(test_name IN LISTS HARNESS_TESTS)
        add_executable(harness_${test_name} harness/tests/${test_name}.c)
        target_link_libraries(harness_${test_name} PRIVATE rigel_harness)
        add_test(NAME harness_${test_name} COMMAND harness_${test_name})
    endforeach()

    # ---------------------------------------------------------------------
    # rigel-harness front-end
    #
    # SDL2 is optional: without it the front-end still builds and runs
    # headless, which is all CI needs.
    # ---------------------------------------------------------------------
    add_executable(rigel-harness harness/main.c harness/harness_diag.c)
    target_link_libraries(rigel-harness PRIVATE rigel_harness m)

    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 QUIET sdl2)
        endif()
    endif()

    if(SDL2_FOUND)
        target_sources(rigel-harness PRIVATE harness/harness_video.c)
        target_compile_definitions(rigel-harness PRIVATE RIGEL_HARNESS_SDL=1)
        if(TARGET SDL2::SDL2)
            target_link_libraries(rigel-harness PRIVATE SDL2::SDL2)
        else()
            target_include_directories(rigel-harness PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(rigel-harness PRIVATE ${SDL2_LIBRARIES})
            target_link_directories(rigel-harness PRIVATE ${SDL2_LIBRARY_DIRS})
        endif()
        message(STATUS "rigel-harness: SDL2 found, interactive video enabled")
    else()
        message(STATUS "rigel-harness: SDL2 not found, headless only")
    endif()
endif()
