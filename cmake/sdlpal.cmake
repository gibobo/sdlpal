file(GLOB SDLPAL_SOURCES	${SDLPAL_DIR}/*.cpp ${SDLPAL_DIR}/*.c ${SDLPAL_DIR}/audio/*.cpp ${SDLPAL_DIR}/audio/*.c)
file(GLOB SDLPAL_HEADERS	${SDLPAL_DIR}/*.hpp ${SDLPAL_DIR}/*.h)

add_library(sdlpal
    STATIC
        ${SDLPAL_SOURCES}
        ${SDLPAL_HEADERS}
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
