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
target_compile_definitions(palreplay PRIVATE
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
# Optional extra arguments: <slot> <file> to start from a copy of a saved game.
function(pal_add_replay scenario work)
    set(save_args "")
    if(ARGC GREATER 2)
        set(save_args -DSAVE_SLOT=${ARGV2} -DSAVE_SOURCE=${ARGV3})
    endif()
    add_test(NAME replay.${scenario}.w${work}
        COMMAND ${CMAKE_COMMAND}
            -DPALREPLAY=$<TARGET_FILE:palreplay>
            -DREPLAYCMP=$<TARGET_FILE:replaycmp>
            -DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/scenarios/${scenario}.txt
            -DBASELINE=${CMAKE_CURRENT_SOURCE_DIR}/baselines/${scenario}.w${work}.txt
            -DOUT=${CMAKE_CURRENT_BINARY_DIR}/${scenario}.w${work}.txt
            -DWORK_MS=${work}
            -DDATA_DIR=${REPLAY_DATA_DIR}
            ${save_args}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_replay.cmake)
    # Scenarios share the data directory, so they must not run in parallel.
    set_tests_properties(replay.${scenario}.w${work} PROPERTIES RESOURCE_LOCK replay_data)
endfunction()

# Scenarios, each without and with 30 ms of simulated work per present.
foreach(work 0 30)
    pal_add_replay(intro ${work})
    pal_add_replay(battle ${work} 1 "${PAL_TEST_RESOURCE_DIR}/0.rpg")
endforeach()
