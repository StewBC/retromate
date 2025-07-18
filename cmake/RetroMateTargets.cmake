include_guard(GLOBAL)

# --- Special function to add SDL2 ---
function(_retromate_add_sdl2_target)
    cmake_parse_arguments(ARG "" "TARGET;VARIANT;PROG_FILE" "SOURCES;COMPILE_DEFINITIONS" ${ARGN})

    if(ARG_VARIANT)
        set(target_variant "${ARG_TARGET}_${ARG_VARIANT}")
    else()
        set(target_variant "${ARG_TARGET}")
    endif()
    set(program_name "${ARG_PROG_FILE}")

    # Required SDL2 packages
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)

    # Executable
    add_executable(${target_variant} ${ARG_SOURCES})
    target_compile_definitions(${target_variant} PRIVATE ${ARG_COMPILE_DEFINITIONS})

    set_target_properties(${target_variant} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        OUTPUT_NAME ${program_name}
    )

    # Link SDL2 libraries, handling static/shared cases smartly
    target_link_libraries(${target_variant}
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
    )

    # Test target to run the app
    add_custom_target(${target_variant}_test
        COMMAND $<TARGET_FILE:${target_variant}>
        DEPENDS ${target_variant}
        COMMENT "Running SDL2 target: $<TARGET_FILE:${target_variant}>"
    )
endfunction()

# --- Special function to add mac68k ---
function(_retromate_add_mac68k_target)
    cmake_parse_arguments(ARG "" "TARGET;VARIANT;PROG_FILE" "SOURCES;COMPILE_DEFINITIONS" ${ARGN})

    if(ARG_VARIANT)
        set(target_variant "${ARG_TARGET}_${ARG_VARIANT}")
    else()
        set(target_variant "${ARG_TARGET}")
    endif()
    set(program_name "${ARG_PROG_FILE}")

    add_application(${target_variant} ${ARG_SOURCES})

    set_target_properties(${target_variant} PROPERTIES COMPILE_OPTIONS -ffunction-sections)
    set_target_properties(${target_variant} PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-strip-macsbug")
endfunction()

# --- _download_ip65(target, out_include, out_libs, out_depfile) ---
function(_download_ip65 TARGET_NAME OUT_INCLUDE OUT_LIBS OUT_DEPFILE)
    set(IP65_LIB ${CMAKE_BINARY_DIR}/ip65)
    set(IP65_ZIP ${IP65_LIB}/ip65-${TARGET_NAME}.zip)
    if("${TARGET_NAME}" STREQUAL "atarixl")
        set(IP65_URL "https://github.com/cc65/ip65/releases/latest/download/ip65-atari.zip")
    else()
        set(IP65_URL "https://github.com/cc65/ip65/releases/latest/download/ip65-${TARGET_NAME}.zip")
    endif()
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

# --- add_retromate_target(...) ---
function(add_retromate_target)
    cmake_parse_arguments(ARG
        ""
        "TARGET;VARIANT;PROG_FILE;DISK_FILE"
        "SOURCES;COMPILE_DEFINITIONS;TEST_USING"
        ${ARGN})

    # Need at least TARGET and SOURCES
    if(NOT ARG_TARGET)
        message(FATAL_ERROR "add_retromate_target: TARGET is required.")
    endif()

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_retromate_target: SOURCES are required.")
    endif()

    set(target ${ARG_TARGET})
    set(variant ${ARG_VARIANT})
    set(target_variant ${target}_${variant})

    # Get names sorted
    if(target STREQUAL "c64")
        set(prog_ext "prg")
        set(disk_ext "d64")
    elseif(target STREQUAL "apple2")
        set(prog_ext "apple2")
        set(disk_ext "po")
    elseif(target STREQUAL "atarixl")
        set(prog_ext "xex")
        set(disk_ext "atr")
    else()
        set(prog_ext "bin")
        set(disk_ext "img")
    endif()

    # Pick a sensible name for binary and disk
    if(NOT ARG_PROG_FILE)
        set(prog_file_name "${PROJECT_NAME}_${variant}.${prog_ext}")
    else()
        set(prog_file_name ${ARG_PROG_FILE})
    endif()

    if(NOT ARG_DISK_FILE)
        set(disk_file_name "${PROJECT_NAME}_${variant}.${disk_ext}")
    else()
        set(disk_file_name ${ARG_DISK_FILE})
    endif()

    # Name the outputs (as in get a variable with the name)
    set(prog_file ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${prog_file_name})
    set(disk_file ${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/${disk_file_name})

    # Some emulators need relative names to work
    file(RELATIVE_PATH PROG_REL_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${prog_file_name}")
    file(RELATIVE_PATH DISK_REL_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/${disk_file_name}")

    # SDL2 goes off here
    if(target STREQUAL "sdl2")
        _retromate_add_sdl2_target(
            TARGET ${target}
            VARIANT ${variant}
            PROG_FILE ${prog_file_name}
            SOURCES "${ARG_SOURCES}"
            COMPILE_DEFINITIONS "${ARG_COMPILE_DEFINITIONS}"
        )
        return()
    elseif(target STREQUAL "mac68k")
        _retromate_add_mac68k_target(
            TARGET ${target}
            VARIANT ${variant}
            PROG_FILE ${prog_file_name}
            SOURCES "${ARG_SOURCES}"
            COMPILE_DEFINITIONS "${ARG_COMPILE_DEFINITIONS}"
        )
        return()
    endif()

    # Only cc65 builds continue below this point
    set(link_flag "")
    set(user_provided_link_cfg FALSE)

    # See if user specified -C in COMPILE_DEFINITIONS
    list(FIND ARG_COMPILE_DEFINITIONS "-C" idx)
    if(NOT idx EQUAL -1)
        set(user_provided_link_cfg TRUE)
    endif()

    # Check for custom cfg file
    set(link_cfg "${CMAKE_SOURCE_DIR}/src/${target}/${target}.cfg")
    if(NOT EXISTS ${link_cfg})
        set(link_cfg "${CMAKE_SOURCE_DIR}/src/${target}/${target}_${variant}.cfg")
    endif()

    # Only use file if user didn't provide -C
    if(EXISTS ${link_cfg} AND NOT user_provided_link_cfg)
        set(link_flag -C ${link_cfg})
    elseif(EXISTS ${link_cfg} AND user_provided_link_cfg)
        message(WARNING
            "You provided '-C' manually in COMPILE_DEFINITIONS, but\n"
            "a default linker config file was also found at '${link_cfg}'.\n"
            "The user-provided setting will take precedence, and the file will be ignored."
        )
    endif()

    # Handle IP65 if needed
    set(IP65_INCLUDE_FLAG "")
    set(IP65_LIBS_FLAG "")
    set(IP65_DEPENDS "")
    if("${variant}" STREQUAL "ip65")
        _download_ip65(${target} ip65_includes ip65_libs ip65_depfile)
        set(IP65_INCLUDE_FLAG -I${ip65_includes})
        set(IP65_LIBS_FLAG ${ip65_libs})
        set(IP65_DEPENDS ${ip65_depfile})
    endif()

    # Compile
    add_custom_command(
        OUTPUT ${prog_file}
        COMMAND ${CMAKE_COMMAND} -E echo "Building ${target_variant}"
        COMMAND cl65 -O -t ${target}
            --mapfile ${target}.map
            ${ARG_COMPILE_DEFINITIONS}
            ${IP65_INCLUDE_FLAG}
            ${ARG_SOURCES}
            ${IP65_LIBS_FLAG}
            ${link_flag}
            -o ${prog_file}
        DEPENDS ${ARG_SOURCES} ${IP65_DEPENDS}
        COMMENT "Building binary: ${prog_file_name}"
        VERBATIM
    )

    # Outcome depends on the disk_file (and disk_file depends on bin_file)
    add_custom_target(${target_variant} ALL
        DEPENDS ${disk_file}
        VERBATIM
    )

    # Make the disks per platform
    if(target STREQUAL "c64")
        get_filename_component(bin_base_name ${prog_file} NAME_WE)
        add_custom_command(
            OUTPUT ${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target_variant}"
            COMMAND ${C1541_EXECUTABLE}
                -format "${PROJECT_NAME},01" d64 ${disk_file}
                -attach ${disk_file}
                -write ${prog_file} ${PROJECT_NAME}
            DEPENDS ${prog_file}
            COMMENT "Disk image: ${disk_file_name}"
            VERBATIM
        )
    elseif(target STREQUAL "apple2")
        add_custom_command(
            OUTPUT ${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target}"
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/assets/prodos.po ${disk_file}
            COMMAND ${CMAKE_COMMAND} -E copy "${Cl65_LOADER_PATH}" "bin/rmate${A2_SLOT}.system#FF2000"
            COMMAND ${CMAKE_COMMAND} -E copy "${prog_file}" "bin/rmate${A2_SLOT}.as"
            COMMAND ${CIDERPRESSII_EXECUTABLE} add --strip-paths "${disk_file}" "bin/rmate${A2_SLOT}.system#FF2000"
            COMMAND ${CIDERPRESSII_EXECUTABLE} add --strip-paths "${disk_file}" "bin/rmate${A2_SLOT}.as"
            COMMAND ${CMAKE_COMMAND} -E rm -f "bin/rmate${A2_SLOT}.as" "bin/rmate${A2_SLOT}.system#FF2000"
            DEPENDS ${prog_file}
            COMMENT "Disk image: ${disk_file_name}"
            VERBATIM
        )
    elseif(target STREQUAL "atarixl")
        add_custom_command(
            OUTPUT ${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Making disk for ${target}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/atarixl"
            COMMAND ${CMAKE_COMMAND} -E copy "${prog_file}" "${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/atarixl/${PROJECT_NAME}"
            COMMAND ${DIR2ATR_EXECUTABLE} -b MyPicoDos406 "${disk_file}" "${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/atarixl"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_DIMAGES_OUTPUT_DIRECTORY}/atarixl"
            DEPENDS ${prog_file}
            COMMENT "Disk image: ${disk_file_name}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${disk_file}
            COMMAND ${CMAKE_COMMAND} -E echo "Not making a disk for unknown target '${target}'"
            COMMENT "No disk image rule for target '${target}'"
            VERBATIM
        )
    endif()

    # Perpare to set up _test targets that will launch the emulator
    # Note - some use the bin_file as it's much faster, but it can easily be changed
    # to testing the disk_file
    if(target STREQUAL "apple2")
        set(test_command ${APPLEWIN_EXECUTABLE} -d1 ${DISK_REL_PATH})
    elseif(target STREQUAL "c64")
        set(test_command ${X64_EXECUTABLE} -autostart ${PROG_REL_PATH})
    elseif(target STREQUAL "atarixl")
        set(test_command ${ATARI_EXECUTABLE} ${PROG_REL_PATH})
    else()
        message(STATUS "No test command available for target '${target}', variant '${variant}'")
    endif()

    # Add the _test target (and it depends on the disk_file to keep it simple)
    if(test_command)
        add_custom_target(${target_variant}_test
            COMMAND ${test_command}
            DEPENDS ${disk_file}
            COMMENT "Running test for ${test_command}"
            VERBATIM
        )
    endif()
endfunction()
