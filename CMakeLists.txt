cmake_minimum_required(VERSION 3.21)

if (${TOOLCHAIN_LANGUAGE} MATCHES "MikroC")
    project(max32690_rtc24_thermo21_oximeter5_ips_dashboard LANGUAGES MikroC)
else()
    project(max32690_rtc24_thermo21_oximeter5_ips_dashboard LANGUAGES C ASM)
endif()

include(mikroeUtils)

if (NOT PROJECT_TYPE)
    set(PROJECT_TYPE "mikrosdk" CACHE STRING "" FORCE)
endif()

add_executable(max32690_rtc24_thermo21_oximeter5_ips_dashboard
    main.c
    sla_header.c
    src/bm_gpio.c
    src/bm_spi.c
    src/bm_i2c_soft.c
    src/buttons.c
    clicks/ips_display2/ips_display2.c
    clicks/rtc24/rtc24.c
    clicks/thermo21/thermo21.c
    clicks/oximeter5/oximeter5.c
    ui/gfx.c
    ui/clock_ui.c
)

target_include_directories(max32690_rtc24_thermo21_oximeter5_ips_dashboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/clicks/ips_display2
    ${CMAKE_CURRENT_SOURCE_DIR}/clicks/rtc24
    ${CMAKE_CURRENT_SOURCE_DIR}/clicks/thermo21
    ${CMAKE_CURRENT_SOURCE_DIR}/clicks/oximeter5
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# Intentionally only Core: no MikroSDK.Driver, HAL, Board, or Click package.
find_package(MikroC.Core REQUIRED)
target_link_libraries(max32690_rtc24_thermo21_oximeter5_ips_dashboard PUBLIC MikroC.Core)

if(COMPILER_REVISION)
    if(${COMPILER_REVISION} VERSION_GREATER_EQUAL "1.0")
        if(${TOOLCHAIN_LANGUAGE} STREQUAL "GNU")
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.meproject)
                fetch_startup_name(${CMAKE_CURRENT_SOURCE_DIR}/.meproject startupName)
                if(startupName)
                    add_startup_all_targets(. ${CMAKE_CURRENT_SOURCE_DIR} ${startupName})
                endif()
                fetch_linker_name(${CMAKE_CURRENT_SOURCE_DIR}/.meproject linkerName)
                if(linkerName)
                    add_ld_all_targets(. ${CMAKE_CURRENT_SOURCE_DIR} ${linkerName})
                endif()
            endif()
        endif()
    endif()
endif()

set(TARGET max32690_rtc24_thermo21_oximeter5_ips_dashboard)

add_custom_command(TARGET ${TARGET} POST_BUILD
    # 1) ELF -> raw bin for signing. Strip the placeholder .sig section
    #    (header + code are already embedded via sla_header.c / the linker script).
    COMMAND ${CMAKE_OBJCOPY}
        -R .sig
        -O binary
        "$<TARGET_FILE:${TARGET}>"
        "$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.bin"

    # 2) Sign - matches the official MaximSDK SBT recipe (SBT-rules.mk): no
    #    header=yes/algo=ecdsa, so sign_app only computes the real signature
    #    (written as a side file "${TARGET}.sig") instead of prepending its
    #    own fallback header.
    COMMAND ${CMAKE_COMMAND} -E env
        MAXIM_SBT_DIR=/home/ivan-ruzavin/git/msdk/Tools/SBT
        /home/ivan-ruzavin/git/msdk/Tools/SBT/bin/sign_app
        -c MAX32690
        key_file=/home/ivan-ruzavin/git/msdk/Tools/SBT/devices/MAX32690/keys/maximtestcrk.key
        ca=$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.bin
        sca=$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.sbin

    # 3) Patch the real signature back into the ELF's .sig section, in place.
    COMMAND ${CMAKE_OBJCOPY}
        --update-section .sig=$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.sig
        "$<TARGET_FILE:${TARGET}>"

    # 4) Final signed hex for the programmer - addresses come straight from
    #    the linker script (0x10000000 header .. 0x10000200 code), no
    #    --change-addresses needed since nothing was prepended externally.
    #    Written under the STANDARD name (${TARGET}.hex) so NECTO Studio's
    #    built-in Flash/Program button (which looks for that name next to
    #    the .elf) picks up the signed image automatically.
    COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        "$<TARGET_FILE:${TARGET}>"
        "$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.hex"

    VERBATIM
)
