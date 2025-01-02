file(GLOB SDLPAL_SOURCES    ${SDLPAL_DIR}/*.c)
file(GLOB SDLPAL_HEADERS	${SDLPAL_DIR}/*.h)
file(GLOB SDLPAL_AUDIO_SOURCES  ${SDLPAL_DIR}/audio/*.c ${SDLPAL_DIR}/audio/*.cpp)
file(GLOB SDLPAL_AUDIO_HEADERS	${SDLPAL_DIR}/audio/*.h)
file(GLOB SDLPAL_VIDEO_SOURCES  ${SDLPAL_DIR}/video/*.c)
file(GLOB SDLPAL_VIDEO_HEADERS	${SDLPAL_DIR}/video/*.h)
file(GLOB SDLPAL_INPUT_SOURCES  ${SDLPAL_DIR}/input/*.c)
file(GLOB SDLPAL_INPUT_HEADERS	${SDLPAL_DIR}/input/*.h)

add_library(sdlpal
    STATIC
    ${SDLPAL_SOURCES}
    ${SDLPAL_HEADERS}
    ${SDLPAL_AUDIO_SOURCES}
    ${SDLPAL_AUDIO_HEADERS}
    ${SDLPAL_VIDEO_SOURCES}
    ${SDLPAL_VIDEO_HEADERS}
    ${SDLPAL_INPUT_SOURCES}
    ${SDLPAL_INPUT_HEADERS}
)

target_include_directories(sdlpal
    PRIVATE
        ${PLATFORM_DIR}
        ${SDL2_DIR}/include
        ${ADPLUG_DIR}
        ${BINIO_INCLUDE_DIR}
        ${SDLPAL_DIR}
)

target_compile_definitions(sdlpal
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)
