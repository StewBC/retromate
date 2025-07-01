# --- Optional: auto-use vcpkg toolchain ---
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

cmake_minimum_required(VERSION 3.15)
project(retromate C)

set(A2_SLOT "3" CACHE STRING "Apple II slot number")

# Find cl65 and figure out where the loader is
find_program(CL65_EXEC cl65 DOC "Available at https://cc65.github.io/")
if(NOT CL65_EXEC)
    message(WARNING "cl65 not found in PATH. Cannot build for Apple 2, C64 or Atari.")
else()
    execute_process(
        COMMAND ${CL65_EXEC} --print-target-path
        OUTPUT_VARIABLE CL65_TARGET_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(LOADER_PATH "${CL65_TARGET_PATH}/apple2/util/loader.system")
    message(STATUS "Path to loader.system=${CL65_TARGET_PATH}/apple2/util/loader.system")
endif()
    
# Tools
find_program(X64_EXECUTABLE NAMES x64sc x64 x64sc.exe x64.exe HINTS ENV VICE_HOME PATHS ENV PATH DOC "Available at https://vice-emu.sourceforge.io/")
message(STATUS "Path to C64 emulator=${X64_EXECUTABLE}")
find_program(C1541_EXECUTABLE c1541 c1541.exe HINTS ENV VICE_HOME PATHS ENV PATH DOC "Available at https://vice-emu.sourceforge.io/")
message(STATUS "Path to c1541=${C1541_EXECUTABLE}")
find_program(CC1541_EXECUTABLE cc1541 cc1541.exe HINTS ENV CC1541_HOME PATHS ENV PATH DOC "Available at https://bitbucket.org/ptv_claus/cc1541")
message(STATUS "Path to cc1541=${CC1541_EXECUTABLE}")
find_program(APPLEWIN_EXECUTABLE applewin AppleWin.exe HINTS ENV APPLEWIN_HOME PATHS ENV PATH DOC "Available at https://github.com/AppleWin/AppleWin")
message(STATUS "Path to Apple 2 emulator=${APPLEWIN_EXECUTABLE}")
find_program(CIDERPRESSII_EXECUTABLE cp2 cp2.exe HINTS ENV CP2_HOME PATHS ENV PATH DOC "Available at https://ciderpress2.com/")
message(STATUS "Path to CiderPress-II=${CIDERPRESSII_EXECUTABLE}")
find_program(ATARI_EXECUTABLE Altirra64 Altirra Altirra64.exe Altirra.exe HINTS ENV ATARI_HOME PATHS ENV PATH DOC "Available at https://www.virtualdub.org/altirra.html")
message(STATUS "Path to Atari emulator=${ATARI_EXECUTABLE}")
find_program(DIR2ATR_EXECUTABLE dir2atr dir2atr.exe HINTS ENV DIR2ATR_HOME PATHS ENV PATH DOC "Available at https://www.horus.com/~hias/atari/")
message(STATUS "Path to dir2atr=${DIR2ATR_EXECUTABLE}")

# Output folders
set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(DISK_DIR ${CMAKE_BINARY_DIR}/disk)
file(MAKE_DIRECTORY ${BIN_DIR} ${DISK_DIR})

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

function(download_ip65 TARGET_NAME OUT_INCLUDE OUT_LIBS OUT_DEPFILE)
    set(IP65_LIB ${CMAKE_BINARY_DIR}/ip65)
    set(IP65_ZIP ${IP65_LIB}/ip65-${TARGET_NAME}.zip)
    set(IP65_URL "https://github.com/cc65/ip65/releases/latest/download/ip65-${TARGET_NAME}.zip")
    set(IP65_LIB_PATH ${IP65_LIB}/ip65_tcp.lib ${IP65_LIB}/ip65_${TARGET_NAME}.lib)
    set(IP65_DEP ${IP65_LIB}/.unpacked_${TARGET_NAME})

    file(MAKE_DIRECTORY ${IP65_LIB})

    add_custom_command(
        OUTPUT ${IP65_DEP}
        COMMAND ${CMAKE_COMMAND} -E echo "Downloading IP65 for ${TARGET_NAME}"
        COMMAND curl -sL -o ${IP65_ZIP} ${IP65_URL}
        COMMAND unzip -o ${IP65_ZIP} -d ${IP65_LIB}
        COMMAND ${CMAKE_COMMAND} -E touch ${IP65_DEP}
        COMMENT "Fetching and unpacking IP65 for ${TARGET_NAME}"
    )

    set(${OUT_INCLUDE} ${IP65_LIB} PARENT_SCOPE)
    set(${OUT_LIBS} ${IP65_LIB_PATH} PARENT_SCOPE)
    set(${OUT_DEPFILE} ${IP65_DEP} PARENT_SCOPE)
endfunction()

function(add_retromate_target target sources bin_file disk_file)
    set(emu_cmd ${ARGN})  # Capture remaining arguments into emu_cmd list

    if(${target} STREQUAL "atarixl")
        download_ip65("atari" IP65_INCLUDE IP65_LIBS IP65_DEP)
    else()
        download_ip65(${target} IP65_INCLUDE IP65_LIBS IP65_DEP)
    endif()

    # Look for optional platform-specific linker config
    set(LINK_CFG "${CMAKE_SOURCE_DIR}/src/${target}/${target}.cfg")
    if(EXISTS ${LINK_CFG})
        message(STATUS "Linker file for ${target} found ${LINK_CFG}")
        set(LINK_CFG_OPTION "-C${LINK_CFG}")
    else()
        message(STATUS "No linker file found for ${target}")
        set(LINK_CFG_OPTION "")
    endif()

    # Extra linker flags just for apple2
    if(${target} STREQUAL "apple2")
        set(EXTRA_LINK_FLAGS "-C" "apple2-hgr.cfg" "-Wl" "-D,__HIMEM__=0xBF00")
        list(APPEND ALL_DEPENDS ${CMAKE_SOURCE_DIR}/src/apple2/retromate.hgr)
    elseif(${target} STREQUAL "c64")
        set(EXTRA_LINK_FLAGS "-Wl" "-D,__HIMEM__=0xC000")
    else()
        set(EXTRA_LINK_FLAGS "")
    endif()

    set(ALL_DEPENDS ${sources} ${IP65_DEP})
    if(EXISTS ${LINK_CFG})
        list(APPEND ALL_DEPENDS ${LINK_CFG})
    endif()

    add_custom_command(
        OUTPUT ${BIN_DIR}/${bin_file}
        COMMAND ${CMAKE_COMMAND} -E echo "Building ${target}"
        COMMAND cl65 -t ${target} --mapfile ${target}.map -O -I${IP65_INCLUDE} ${EXTRA_LINK_FLAGS} ${sources} ${IP65_LIBS} ${LINK_CFG_OPTION} -o ${BIN_DIR}/${bin_file}
        DEPENDS ${ALL_DEPENDS}
        COMMENT "Building ${target}"
        VERBATIM
    )

    if(${target} STREQUAL "apple2")
        add_custom_command(
            OUTPUT ${DISK_DIR}/${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target}"

            # Copy template disk image (cross-platform)
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/prodos.po ${DISK_DIR}/${disk_file}

            # Copy the files with names for the tool (.as = apple single; #xxyyyy for type and aux)
            COMMAND ${CMAKE_COMMAND} -E copy "${LOADER_PATH}" "bin/rmate${A2_SLOT}.system#FF2000"
            COMMAND ${CMAKE_COMMAND} -E copy "${BIN_DIR}/${bin_file}" "bin/rmate${A2_SLOT}.as"

            # Add to disk (no path!)
            COMMAND ${CIDERPRESSII_EXECUTABLE} add --strip-paths "${DISK_DIR}/${disk_file}" "bin/rmate${A2_SLOT}.system#FF2000"
            COMMAND ${CIDERPRESSII_EXECUTABLE} add --strip-paths "${DISK_DIR}/${disk_file}" "bin/rmate${A2_SLOT}.as"

            # Clean up renamed/attribute-tagged files for Cadius
            COMMAND ${CMAKE_COMMAND} -E rm -f "bin/rmate${A2_SLOT}.as" "bin/rmate${A2_SLOT}.system#FF2000"

            DEPENDS ${BIN_DIR}/${bin_file}
            COMMENT "Disk image: ${disk_file}"
            VERBATIM
        )
    elseif(${target} STREQUAL "c64")
        # Build the command now that we have disk_file and bin_file
        set(C64DISK_COMMAND)
        if(C1541_EXECUTABLE)
            get_filename_component(bin_file_no_ext "${bin_file}" NAME_WE)
            list(APPEND C64DISK_COMMAND
                ${C1541_EXECUTABLE}
                -format "retromate,01" d64 "${DISK_DIR}/${disk_file}"
                -attach "${DISK_DIR}/${disk_file}"
                -write "${BIN_DIR}/${bin_file}" "${bin_file_no_ext}"
            )
        elseif(CC1541_EXECUTABLE)
            list(APPEND C64DISK_COMMAND
                ${CC1541_EXECUTABLE}
                -q
                -f retromate
                -w "${BIN_DIR}/${bin_file}" "${DISK_DIR}/${disk_file}"
            )
        else()
            message(FATAL_ERROR "Neither c1541 nor cc1541 was found.")
        endif()

        add_custom_command(
            OUTPUT ${DISK_DIR}/${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target}"
            COMMAND ${C64DISK_COMMAND}
            DEPENDS ${BIN_DIR}/${bin_file}
            COMMENT "Disk image: ${disk_file}"
            VERBATIM
        )
    elseif(${target} STREQUAL "atarixl")
        add_custom_command(
            OUTPUT ${DISK_DIR}/${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target}"

            # Make a folder to become the Atari disk
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DISK_DIR}/atarixl"

            # Copy to folder
            COMMAND ${CMAKE_COMMAND} -E copy "${BIN_DIR}/${bin_file}" "${DISK_DIR}/atarixl/rtrmate"

            # Make the disk
            # COMMAND ${DIR2ATR_EXECUTABLE} -ab MyPicoDos406 "${DISK_DIR}/${disk_file}" "${DISK_DIR}/atarixl"
            COMMAND ${DIR2ATR_EXECUTABLE} -b MyPicoDos406 "${DISK_DIR}/${disk_file}" "${DISK_DIR}/atarixl"

            # Clean up renamed/attribute-tagged files for Cadius (remove file)
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${DISK_DIR}/atarixl"

            DEPENDS ${BIN_DIR}/${bin_file}
            COMMENT "Disk image: ${disk_file}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${DISK_DIR}/${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Not making a disk for unknown target '${target}'"
            COMMENT "No disk image rule for target '${target}'"
            VERBATIM
        )
    endif()

    add_custom_target(${target} ALL
        DEPENDS ${DISK_DIR}/${disk_file}
    )

    if(emu_cmd)
        add_custom_target(${target}_test
            COMMAND ${emu_cmd}
            DEPENDS ${target}
            COMMENT "Running ${target} in emulator"
        )
    endif()
endfunction()

# --- C64 ---
set(C64_SOURCES
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/c64/dataC64.c
    ${CMAKE_SOURCE_DIR}/src/c64/hiresC64.s
    ${CMAKE_SOURCE_DIR}/src/c64/platC64core.c
    ${CMAKE_SOURCE_DIR}/src/c64/platC64draw.c
    ${CMAKE_SOURCE_DIR}/src/c64/platC64net.c
)

add_retromate_target(
    c64
    "${C64_SOURCES}"
    retromate.prg
    retromate.d64
    ${X64_EXECUTABLE} -autostart ${BIN_DIR}/retromate.prg
    # ${X64_EXECUTABLE} -autostart ${DISK_DIR}/retromate.d64
)

# --- Apple II ---
set(APPLE2_SOURCES
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/apple2/hgr.s
    ${CMAKE_SOURCE_DIR}/src/apple2/hiresA2.s
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2core.c
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2draw.c
    ${CMAKE_SOURCE_DIR}/src/apple2/platA2net.c
)

file(RELATIVE_PATH DISK_REL_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${DISK_DIR}/retromate")

add_retromate_target(
    apple2
    "${APPLE2_SOURCES}"
    retromate.apple2
    retromate.po
    ${APPLEWIN_EXECUTABLE} -d1 ${DISK_REL_PATH}.po
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

add_retromate_target(
    atarixl
    "${ATARI_SOURCES}"
    retromate.xex
    retromate.atr
    ${ATARI_EXECUTABLE} ${BIN_DIR}/retromate.xex
    # ${ATARI_EXECUTABLE} ${DISK_DIR}/retromate.atr
)

# --- Native SDL2 ---
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)

add_executable(sdl2
    ${SHARED_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2core.c
    ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2draw.c
    ${CMAKE_SOURCE_DIR}/src/sdl2/platSDL2net.c
)

set_target_properties(sdl2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set_target_properties(sdl2 PROPERTIES OUTPUT_NAME retromate)

target_link_libraries(sdl2
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
)

add_custom_target(sdl2_test
    COMMAND $<TARGET_FILE:sdl2>
    DEPENDS sdl2
    COMMENT "Running SDL2 native test: $<TARGET_FILE:sdl2>"
)