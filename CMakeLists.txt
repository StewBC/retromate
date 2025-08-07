cmake_minimum_required(VERSION 3.21)
project(retromate C)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_DIMAGES_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dsk)   # Custom

# Make dsk folder, others made automatically
file(MAKE_DIRECTORY ${CMAKE_DIMAGES_OUTPUT_DIRECTORY})

# Required tools for disk images and emulators
find_program(X64_EXECUTABLE NAMES x64sc x64 x64sc.exe x64.exe HINTS ENV VICE_HOME PATHS ENV PATH DOC "VICE emulator available at https://vice-emu.sourceforge.io/")
find_program(C1541_EXECUTABLE NAMES c1541 c1541.exe HINTS ENV VICE_HOME PATHS ENV PATH DOC "VICE disk tool available at https://vice-emu.sourceforge.io/")
find_program(CC1541_EXECUTABLE NAMES cc1541 cc1541.exe HINTS ENV CC1541_HOME PATHS ENV PATH DOC "Alternate C64 disk builder available at https://bitbucket.org/ptv_claus/cc154")
find_program(APPLEWIN_EXECUTABLE NAMES applewin AppleWin.exe HINTS ENV APPLEWIN_HOME PATHS ENV PATH DOC "Apple II emulator available at https://github.com/AppleWin/AppleWin")
find_program(CIDERPRESSII_EXECUTABLE NAMES cp2 cp2.exe HINTS ENV CP2_HOME PATHS ENV PATH DOC "CiderPress-II available at https://ciderpress2.com/")
find_program(ATARI_EXECUTABLE NAMES Altirra64 Altirra Altirra64.exe Altirra.exe HINTS ENV ATARI_HOME PATHS ENV PATH DOC "Atari emulator available at https://www.virtualdub.org/altirra.html")
find_program(DIR2ATR_EXECUTABLE NAMES dir2atr dir2atr.exe HINTS ENV DIR2ATR_HOME PATHS ENV PATH DOC "Atari disk packer available at https://www.horus.com/~hias/atari/")
find_program(CL65_EXECUTABLE cl65 DOC "Available at https://cc65.github.io/")

message(STATUS "C64 emulator: ${X64_EXECUTABLE}")
message(STATUS "C1541: ${C1541_EXECUTABLE}")
message(STATUS "CC1541: ${CC1541_EXECUTABLE}")
message(STATUS "AppleWin: ${APPLEWIN_EXECUTABLE}")
message(STATUS "CiderPressII: ${CIDERPRESSII_EXECUTABLE}")
message(STATUS "Atari emulator: ${ATARI_EXECUTABLE}")
message(STATUS "Dir2atr: ${DIR2ATR_EXECUTABLE}")
message(STATUS "cl65: ${CL65_EXECUTABLE}")

# Find out where the cc65 loader program is found
if(CL65_EXECUTABLE)
    execute_process(
        COMMAND ${CL65_EXECUTABLE} --print-target-path
        OUTPUT_VARIABLE CL65_TARGET_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(Cl65_LOADER_PATH "${CL65_TARGET_PATH}/apple2/util/loader.system")
    message(STATUS "Path to loader.system=${Cl65_LOADER_PATH}")
endif()

# Include the target creation logic
include(${CMAKE_SOURCE_DIR}/cmake/RetroMateTargets.cmake)

# Shared sources
set(SHARED_SOURCES
    ${CMAKE_SOURCE_DIR}/src/app.c
    ${CMAKE_SOURCE_DIR}/src/fics.c
    ${CMAKE_SOURCE_DIR}/src/global.c
    ${CMAKE_SOURCE_DIR}/src/log.c
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${CMAKE_SOURCE_DIR}/src/menu.c
    ${CMAKE_SOURCE_DIR}/src/ui.c
    ${CMAKE_SOURCE_DIR}/src/usrinput.c
)

# -- c64 ---
set(C64_SOURCES
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/c64/dataC64.c
    ${CMAKE_SOURCE_DIR}/src/c64/hiresC64.s
    ${CMAKE_SOURCE_DIR}/src/c64/mouse_drv.s
    ${CMAKE_SOURCE_DIR}/src/c64/platC64core.c
    ${CMAKE_SOURCE_DIR}/src/c64/platC64draw.c
    ${CMAKE_SOURCE_DIR}/src/c64/platC64net.c
)

set(C64_TR_SOURCES
    ${C64_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/c64/swlinkC64.s
)

# c64 ip65
add_retromate_target(
    TARGET c64
    VARIANT ip65
    SOURCES ${C64_SOURCES}
    COMPILE_DEFINITIONS -DUSE_IP65 -Wl -D,__HIMEM__=0xC000
)

# c64 teensyrom
add_retromate_target(
    TARGET c64
    VARIANT teensyrom
    SOURCES ${C64_TR_SOURCES}
    COMPILE_DEFINITIONS -DUSE_TR -Wl -D,__HIMEM__=0xC000
)

# --- Apple II ---
set(A2_SLOT "3" CACHE STRING "Apple II slot number")

set(APPLE2_SOURCES
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/apple2/hgr.s
    ${CMAKE_SOURCE_DIR}/src/apple2/hiresA2.s
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2core.c
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2draw.c
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2net.c
)

# Apple II ip65
add_retromate_target(
    TARGET apple2
    VARIANT ip65
    SOURCES ${APPLE2_SOURCES}
    COMPILE_DEFINITIONS -C apple2-hgr.cfg -Wl -D,__HIMEM__=0xBF00
)

# --- atarixl ---
set(ATARI_SOURCES
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/atarixl/dataAtari.c
    ${CMAKE_SOURCE_DIR}/src/atarixl/hiresAtari.s
    ${CMAKE_SOURCE_DIR}/src/atarixl/platAtaricore.c
    ${CMAKE_SOURCE_DIR}/src/atarixl/platAtaridraw.c
    ${CMAKE_SOURCE_DIR}/src/atarixl/platAtarinet.c
)

# atarixl ip65
add_retromate_target(
    TARGET atarixl
    VARIANT ip65
    SOURCES ${ATARI_SOURCES}
)

# SDL2 or Mac68K depending on compiler
if(NOT CMAKE_SYSTEM_NAME MATCHES Retro68)
    # --- SDL2 ---
    message(STATUS "Including SDL2 target")
    set(SDL2_SOURCES
        ${SHARED_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2core.c
        ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2draw.c
        ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2net.c
    )

    # SLD2 no variant
    add_retromate_target(
        TARGET sdl2
        SOURCES ${SDL2_SOURCES}
        PROG_FILE retromate
        COMPILE_DEFINITIONS -DUSE_SDL2
    )
else()
    # --- Mac68K ---
    message(STATUS "Including Mac68K target")
    set(MAC68K_SOURCES
        ${SHARED_SOURCES}
        ${CMAKE_SOURCE_DIR}/src/mac68k/platmac68kcore.c
        ${CMAKE_SOURCE_DIR}/src/mac68k/platmac68kdraw.c
        ${CMAKE_SOURCE_DIR}/src/mac68k/platmac68knet.c
    )

    # Mac68K no variant
    add_retromate_target(
        TARGET mac68k
        SOURCES ${MAC68K_SOURCES}
        PROG_FILE retromate
        COMPILE_DEFINITIONS -DARGS_CREATOR=SWRM
    )
endif()
