add_library(timidity
    ${TIMIDITY_DIR}/common.c
    ${TIMIDITY_DIR}/instrum.c
    ${TIMIDITY_DIR}/mix.c
    ${TIMIDITY_DIR}/output.c
    ${TIMIDITY_DIR}/playmidi.c
    ${TIMIDITY_DIR}/readmidi.c
    ${TIMIDITY_DIR}/resample.c
    ${TIMIDITY_DIR}/tables.c
    ${TIMIDITY_DIR}/timidity.c
)

target_include_directories(timidity
    PRIVATE
        ${SDL2_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_DIR}
        ${TIMIDITY_DIR}
)
