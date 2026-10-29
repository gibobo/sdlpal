cmake_minimum_required(VERSION 3.16)
project(core_diff LANGUAGES C)

# Differential tests of src/ against the upstream implementation. The upstream
# sources are read from git at configure time; nothing upstream is vendored.
get_filename_component(PAL_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(PAL_MASTER_REF "d4bf69b" CACHE STRING "Upstream commit the core-diff tests compare against")
set(PAL_TEST_RESOURCE_DIR "${PAL_REPO_ROOT}/Pal98rqptw" CACHE PATH "Directory with the complete WIN95 game data")
include(${CMAKE_CURRENT_SOURCE_DIR}/MasterSource.cmake)

# Reject a missing or incomplete data set before building anything.
foreach(data_file abc.mkf ball.mkf data.mkf f.mkf fbp.mkf fire.mkf gop.mkf map.mkf mgo.mkf rgm.mkf rng.mkf sss.mkf)
    if(NOT EXISTS "${PAL_TEST_RESOURCE_DIR}/${data_file}")
        message(FATAL_ERROR "core-diff: ${PAL_TEST_RESOURCE_DIR}/${data_file} not found; set PAL_TEST_RESOURCE_DIR")
    endif()
endforeach()

# Resolve the upstream commit so test output names exactly what it compared with.
execute_process(
    COMMAND git -C "${PAL_REPO_ROOT}" rev-parse --short "${PAL_MASTER_REF}^{commit}"
    OUTPUT_VARIABLE PAL_MASTER_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE rev_result)
if(NOT rev_result EQUAL 0)
    message(FATAL_ERROR "core-diff: unknown PAL_MASTER_REF '${PAL_MASTER_REF}'; run 'git fetch origin master'")
endif()
message(STATUS "core-diff: comparing against upstream ${PAL_MASTER_COMMIT}")

# Extract whole upstream files and slice single functions out of larger ones.
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
pal_git_export("${PAL_MASTER_REF}" yj1.c ${GEN_DIR}/master/yj1.c)
pal_git_export("${PAL_MASTER_REF}" palcommon.c ${GEN_DIR}/master/palcommon.c)
pal_git_export("${PAL_MASTER_REF}" palcommon.h ${GEN_DIR}/master/palcommon.h)

pal_git_show("${PAL_MASTER_REF}" util.c master_util_text)
set(master_util_slice "")
pal_slice_line(master_util_text "static int glSeed" master_util_slice)
pal_slice_functions(master_util_text master_util_slice lsrand lrand RandomLong RandomFloat)
file(WRITE ${GEN_DIR}/master/util_rng.inc "${master_util_slice}")

pal_git_show("${PAL_MASTER_REF}" fight.c master_fight_text)
set(master_fight_slice "")
pal_slice_functions(master_fight_text master_fight_slice PAL_CalcBaseDamage PAL_CalcMagicDamage PAL_CalcPhysicalAttackDamage)
file(WRITE ${GEN_DIR}/master/fight_damage.inc "${master_fight_slice}")

pal_git_show("${PAL_MASTER_REF}" rngplay.c master_rngplay_text)
set(master_rngplay_slice "")
pal_slice_functions(master_rngplay_text master_rngplay_slice PAL_RNGBlitToSurface)
file(WRITE ${GEN_DIR}/master/rngplay_blit.inc "${master_rngplay_slice}")

# Slice the file-static development functions from the working tree.
file(READ ${PAL_REPO_ROOT}/src/fight.c dev_fight_text)
set(dev_fight_slice "")
pal_slice_functions(dev_fight_text dev_fight_slice PAL_CalcBaseDamage PAL_CalcMagicDamage PAL_CalcPhysicalAttackDamage)
file(WRITE ${GEN_DIR}/dev/fight_damage.inc "${dev_fight_slice}")

file(READ ${PAL_REPO_ROOT}/src/rngplay.c dev_rngplay_text)
set(dev_rngplay_slice "")
pal_slice_functions(dev_rngplay_text dev_rngplay_slice PAL_RNGBlitToSurface)
file(WRITE ${GEN_DIR}/dev/rngplay_blit.inc "${dev_rngplay_slice}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${PAL_REPO_ROOT}/src/fight.c ${PAL_REPO_ROOT}/src/rngplay.c)

# Force the master_ prefix onto every upstream translation unit, and silence the
# warnings of upstream code that this project does not maintain.
if(MSVC)
    set(MASTER_RENAME_OPTION /FI${CMAKE_CURRENT_SOURCE_DIR}/master_rename.h /w)
else()
    set(MASTER_RENAME_OPTION -include ${CMAKE_CURRENT_SOURCE_DIR}/master_rename.h -w)
endif()

# Upstream code compiled against the shim headers.
add_library(core_diff_master STATIC
    ${GEN_DIR}/master/yj1.c
    ${GEN_DIR}/master/palcommon.c
    master_support.c
    master_util.c
    master_rngblit.c)
target_include_directories(core_diff_master PRIVATE
    ${GEN_DIR}/master ${CMAKE_CURRENT_SOURCE_DIR}/shim ${CMAKE_CURRENT_SOURCE_DIR}/include ${GEN_DIR})
target_compile_options(core_diff_master PRIVATE ${MASTER_RENAME_OPTION})

# Upstream damage formulas compiled against the development game data structures.
add_library(core_diff_master_fight STATIC master_fight.c)
target_include_directories(core_diff_master_fight PRIVATE
    ${PAL_REPO_ROOT}/src ${CMAKE_CURRENT_SOURCE_DIR}/include ${GEN_DIR})
target_compile_options(core_diff_master_fight PRIVATE ${MASTER_RENAME_OPTION})

# Development code under test, plus slices of its file-static functions.
add_library(core_diff_dev STATIC
    ${PAL_REPO_ROOT}/src/palcommon.c
    ${PAL_REPO_ROOT}/src/yj1.c
    ${PAL_REPO_ROOT}/src/util.c
    ${PAL_REPO_ROOT}/src/util_clock.c
    ${PAL_REPO_ROOT}/src/util_io.c
    ${PAL_REPO_ROOT}/src/util_memory.c
    dev_slices.c
    stubs.c)
target_include_directories(core_diff_dev PRIVATE ${PAL_REPO_ROOT}/src ${GEN_DIR})

add_executable(core_diff core_diff.c)
target_include_directories(core_diff PRIVATE ${PAL_REPO_ROOT}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(core_diff PRIVATE
    PAL_TEST_RESOURCE_DIR="${PAL_TEST_RESOURCE_DIR}"
    PAL_MASTER_COMMIT="${PAL_MASTER_COMMIT}")
if(MSVC)
    target_compile_definitions(core_diff PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
target_link_libraries(core_diff PRIVATE core_diff_dev core_diff_master core_diff_master_fight core_diff_dev)

enable_testing()
foreach(test_name mkf_read yj2_chunks rle_blits xy_offset rng_stream damage rng_blit)
    add_test(NAME core_diff.${test_name} COMMAND core_diff ${test_name})
endforeach()
