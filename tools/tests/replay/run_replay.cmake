# Run one replay scenario and compare its trace with the baseline. With
# PAL_REPLAY_RECORD=1 in the environment the trace replaces the baseline instead.
# Inputs: PALREPLAY REPLAYCMP SCRIPT BASELINE OUT WORK_MS DATA_DIR [SAVE_SLOT SAVE_SOURCE]

# Start every run from the same saved games.
foreach(slot 1 2 3 4 5)
    file(REMOVE "${DATA_DIR}/${slot}.rpg")
endforeach()
if(DEFINED SAVE_SLOT)
    configure_file("${SAVE_SOURCE}" "${DATA_DIR}/${SAVE_SLOT}.rpg" COPYONLY)
endif()

# Play the scenario headless.
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env PAL_REPLAY_SCRIPT=${SCRIPT} PAL_REPLAY_OUT=${OUT} PAL_REPLAY_WORK_MS=${WORK_MS}
        ${PALREPLAY}
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_log
    ERROR_VARIABLE run_log)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "palreplay failed (${run_result}):\n${run_log}")
endif()

# Record a new baseline on request.
if("$ENV{PAL_REPLAY_RECORD}" STREQUAL "1")
    configure_file("${OUT}" "${BASELINE}" COPYONLY)
    message(STATUS "recorded ${BASELINE}")
    return()
endif()

# Otherwise require the trace to match the baseline exactly.
if(NOT EXISTS "${BASELINE}")
    message(FATAL_ERROR "no baseline ${BASELINE}; record one with PAL_REPLAY_RECORD=1")
endif()
execute_process(
    COMMAND ${REPLAYCMP} ${BASELINE} ${OUT}
    RESULT_VARIABLE compare_result
    OUTPUT_VARIABLE compare_log)
message(STATUS "${compare_log}")
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "replay diverged from ${BASELINE}")
endif()
