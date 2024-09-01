file(GLOB SDLPAL_SOURCES	${SDLPAL_DIR}/*.cpp ${SDLPAL_DIR}/*.c)
file(GLOB SDLPAL_HEADERS	${SDLPAL_DIR}/*.hpp ${SDLPAL_DIR}/*.h)

add_library(sdlpal
    STATIC
        ${SDLPAL_SOURCES}
        ${SDLPAL_HEADERS}
)

target_include_directories(sdlpal
    PRIVATE
        win32
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${BINIO_INCLUDE_DIR}
)

target_compile_definitions(sdlpal
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        PAL_HAS_PLATFORM_SPECIFIC_UTILS
        # PAL_CLASSIC
        # ENABLE_REVISIED_BATTLE
)
