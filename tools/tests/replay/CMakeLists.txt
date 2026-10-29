# Deterministic replay of scripted sessions on the development core: the game
# runs headless on a virtual clock with scripted input, and every presented
# frame is reduced to state, screen and palette hashes compared with baselines.
# Built from the root project (it needs GAME_SOURCES) with -DPAL_BUILD_TESTS=ON.
if(NOT GAME_SOURCES)
    message(FATAL_ERROR "tools/tests/replay must be added from the root CMakeLists.txt")
endif()
get_filename_component(PAL_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(PAL_TEST_RESOURCE_DIR "${PAL_REPO_ROOT}/Pal98rqptw" CACHE PATH "Directory with the complete WIN95 game data")

# Give the replays a private copy of the game data, since scenarios load and
# write saved games next to it.
set(REPLAY_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
foreach(resource abc ball data f fbp fire gop map mgo mus pat rgm rng sounds sss)
    if(NOT EXISTS "${PAL_TEST_RESOURCE_DIR}/${resource}.mkf")
        message(FATAL_ERROR "replay: ${PAL_TEST_RESOURCE_DIR}/${resource}.mkf not found; set PAL_TEST_RESOURCE_DIR")
    endif()
    file(COPY "${PAL_TEST_RESOURCE_DIR}/${resource}.mkf" DESTINATION ${REPLAY_DATA_DIR})
endforeach()

# Drop the root data paths for this directory; the replay targets define their own.
# (Undefining them with /U does not work: MSVC sees target /D options before /U.)
get_directory_property(replay_definitions COMPILE_DEFINITIONS)
list(FILTER replay_definitions EXCLUDE REGEX "^(RESOURCE_PATH|CACHES_PATH)=")
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "${replay_definitions}")

# The game core with the virtual clock instead of src/util_clock.c.
set(REPLAY_GAME_SOURCES ${GAME_SOURCES})
list(FILTER REPLAY_GAME_SOURCES EXCLUDE REGEX "/util_clock\\.c$")

add_executable(palreplay
    ${REPLAY_GAME_SOURCES}
    ${PAL_REPO_ROOT}/driver/DrvIf.c
    ${PAL_REPO_ROOT}/driver/Audio_Dummy.c
    Event_Replay.c
    Video_Replay.c
    vclock.c
    state_hash.c)
target_include_directories(palreplay PRIVATE ${PAL_REPO_ROOT}/src ${CMAKE_CURRENT_SOURCE_DIR})
# The measurement hooks only count and time; with the virtual clock and scripted
# input the counters printed at the end of a run are deterministic.
target_compile_definitions(palreplay PRIVATE
    PAL_PROFILE=1
    RESOURCE_PATH="${REPLAY_DATA_DIR}"
    CACHES_PATH="${PAL_REPO_ROOT}/cache_data")

add_executable(replaycmp replaycmp.c)

# Timing checks of transitions and frame pacing; has its own main and drivers.
add_executable(paltiming
    ${REPLAY_GAME_SOURCES}
    ${PAL_REPO_ROOT}/driver/DrvIf.c
    ${PAL_REPO_ROOT}/driver/Audio_Dummy.c
    paltiming.c
    vclock.c)
target_include_directories(paltiming PRIVATE ${PAL_REPO_ROOT}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(paltiming PRIVATE
    PAL_NO_MAIN
    RESOURCE_PATH="${REPLAY_DATA_DIR}"
    CACHES_PATH="${PAL_REPO_ROOT}/cache_data")
foreach(work 0 30)
    add_test(NAME paltiming.w${work} COMMAND paltiming ${work})
    set_tests_properties(paltiming.w${work} PROPERTIES RESOURCE_LOCK replay_data)
endforeach()

if(MSVC)
    target_compile_definitions(palreplay PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(replaycmp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(paltiming PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Register replay.<scenario>.w<work>: run the scenario with <work> virtual
# milliseconds of simulated work per present and compare with its baseline.
# SAVE <slot> <file> starts from a copy of a saved game; CACHE <bytes> runs with
# that chunk cache budget (test name suffix .cache<bytes>), which must not change
# the trace.
function(pal_add_replay scenario work)
    cmake_parse_arguments(REPLAY "" "CACHE" "SAVE" ${ARGN})
    set(name replay.${scenario}.w${work})
    set(extra_args "")
    if(REPLAY_SAVE)
        list(GET REPLAY_SAVE 0 save_slot)
        list(GET REPLAY_SAVE 1 save_source)
        list(APPEND extra_args -DSAVE_SLOT=${save_slot} -DSAVE_SOURCE=${save_source})
    endif()
    if(DEFINED REPLAY_CACHE)
        set(name ${name}.cache${REPLAY_CACHE})
        list(APPEND extra_args -DCACHE_BYTES=${REPLAY_CACHE})
    endif()
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DPALREPLAY=$<TARGET_FILE:palreplay>
            -DREPLAYCMP=$<TARGET_FILE:replaycmp>
            -DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/scenarios/${scenario}.txt
            -DBASELINE=${CMAKE_CURRENT_SOURCE_DIR}/baselines/${scenario}.w${work}.txt
            -DOUT=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt
            -DWORK_MS=${work}
            -DDATA_DIR=${REPLAY_DATA_DIR}
            ${extra_args}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_replay.cmake)
    # Scenarios share the data directory, so they must not run in parallel.
    set_tests_properties(${name} PROPERTIES RESOURCE_LOCK replay_data)
endfunction()

# Scenarios, each without and with 30 ms of simulated work per present.
foreach(work 0 30)
    pal_add_replay(intro ${work})
    pal_add_replay(battle ${work} SAVE 1 "${PAL_TEST_RESOURCE_DIR}/0.rpg")
endforeach()

# The chunk cache only saves reads: no cache and a cache under constant
# eviction must give the same traces.
foreach(cache 0 65536)
    pal_add_replay(intro 0 CACHE ${cache})
    pal_add_replay(battle 0 SAVE 1 "${PAL_TEST_RESOURCE_DIR}/0.rpg" CACHE ${cache})
endforeach()
