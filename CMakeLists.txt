cmake_minimum_required(VERSION 3.21)
project(kof2000 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The neogeorecomp toolkit: a pinned submodule (see .gitmodules and setup.sh).
# A sibling checkout is accepted too, for development.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../ironclad/neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

# Your ROM files (the MAME "kof2000" set plus the "neogeo" system ROMs,
# unzipped). Without it the player builds interpreter-only.
set(KOF2000_ROM_DIR "" CACHE PATH "Directory with your KOF2000 and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(KOF2000_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

# Stage 0: ROM decryption (NEO-CMC50 graphics + audio). The program ROMs
# (257-pg1.p1/257-pg2.sp2) are already de-protected and are copied through.
add_executable(kof2000_decrypt tools/decrypt.c)

# Stage 1: the generator.
add_executable(kof2000_recomp src/recomp_main.c src/game.c)
target_link_libraries(kof2000_recomp PRIVATE m68krecomp_core)
target_include_directories(kof2000_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 16)
if(KOF2000_ROM_DIR)
    set(DEC_DIR ${CMAKE_BINARY_DIR}/roms)
    set(DEC_STAMP ${DEC_DIR}/.decrypted)
    add_custom_command(
        OUTPUT ${DEC_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEC_DIR}
        COMMAND kof2000_decrypt --rom-path ${KOF2000_ROM_DIR} --out ${DEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${DEC_STAMP}
        DEPENDS kof2000_decrypt
        COMMENT "Decrypting KOF2000 ROMs (NEO-CMC50)")

    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from a
    # scripted play session and the next build regenerates with it.
    if(NOT EXISTS ${KOF2000_ENTRIES})
        file(WRITE ${KOF2000_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND kof2000_recomp --rom-path ${DEC_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${KOF2000_ENTRIES}
        DEPENDS kof2000_recomp ${KOF2000_ENTRIES} ${DEC_STAMP}
        COMMENT "Recompiling KOF2000 from ${DEC_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(kof2000 ${GAME_SOURCES} src/game.c)
set_target_properties(kof2000 PROPERTIES OUTPUT_NAME "KOF2000")
target_link_libraries(kof2000 PRIVATE neogeorecomp)
target_include_directories(kof2000 PRIVATE src ${GEN_DIR})
if(KOF2000_ROM_DIR)
    add_custom_target(profile
        COMMAND kof2000 --headless --rom-path ${DEC_DIR} --frames 6000 --dump-misses ${KOF2000_ENTRIES}
        COMMAND kof2000 --headless --rom-path ${DEC_DIR} --frames 19000
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/coin_start.txt --dump-misses ${KOF2000_ENTRIES}
        DEPENDS kof2000
        COMMENT "Profiling a scripted session for runtime-only entry points")
    target_compile_definitions(kof2000 PRIVATE KOF2000_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O2 makes MSVC's optimizer take tens of minutes on these giant
        # dispatch functions (it is superlinear in function size), while /O1
        # finishes in seconds and runs the 68000 code at full speed.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1")
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        # Same reason: keep the generated C cheap to compile.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1")
    endif()
endif()
if(MSVC)
    target_compile_definitions(kof2000 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(kof2000_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(kof2000_decrypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET kof2000 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:kof2000> $<TARGET_FILE_DIR:kof2000>
        COMMAND_EXPAND_LISTS)
endif()
