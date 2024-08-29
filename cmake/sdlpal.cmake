set(SDLPAL_SOURCE_FILES
    ${SDLPAL_SOURCE_DIR}/audio.c
    ${SDLPAL_SOURCE_DIR}/battle.c
    ${SDLPAL_SOURCE_DIR}/ending.c
    ${SDLPAL_SOURCE_DIR}/fight.c
    ${SDLPAL_SOURCE_DIR}/font.c
    ${SDLPAL_SOURCE_DIR}/game.c
    ${SDLPAL_SOURCE_DIR}/global.c
    ${SDLPAL_SOURCE_DIR}/glslp.c
    ${SDLPAL_SOURCE_DIR}/input.c
    ${SDLPAL_SOURCE_DIR}/itemmenu.c
    ${SDLPAL_SOURCE_DIR}/magicmenu.c
    ${SDLPAL_SOURCE_DIR}/map.c
    ${SDLPAL_SOURCE_DIR}/mini_glloader.c
    ${SDLPAL_SOURCE_DIR}/palcfg.c
    ${SDLPAL_SOURCE_DIR}/palcommon.c
    ${SDLPAL_SOURCE_DIR}/palette.c
    ${SDLPAL_SOURCE_DIR}/play.c
    ${SDLPAL_SOURCE_DIR}/res.c
    ${SDLPAL_SOURCE_DIR}/resampler.c
    ${SDLPAL_SOURCE_DIR}/rixplay.cpp
    ${SDLPAL_SOURCE_DIR}/rngplay.c
    ${SDLPAL_SOURCE_DIR}/scene.c
    ${SDLPAL_SOURCE_DIR}/script.c
    ${SDLPAL_SOURCE_DIR}/sound.c
    ${SDLPAL_SOURCE_DIR}/text.c
    ${SDLPAL_SOURCE_DIR}/ui.c
    ${SDLPAL_SOURCE_DIR}/uibattle.c
    ${SDLPAL_SOURCE_DIR}/uigame.c
    ${SDLPAL_SOURCE_DIR}/util.c
    ${SDLPAL_SOURCE_DIR}/video.c
    ${SDLPAL_SOURCE_DIR}/video_glsl.c
    ${SDLPAL_SOURCE_DIR}/yj1.c
)

add_library(sdlpal
    STATIC
        ${SDLPAL_SOURCE_FILES}
)

target_include_directories(sdlpal
    PRIVATE
        # ${CMAKE_SOURCE_DIR}
        win32
        ${SDLPAL_INCLUDE_DIR}
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${BINIO_INCLUDE_DIR}
)

# target_compile_definitions(sdlpal
#     PRIVATE
#     PAL_CLASSIC
#     ENABLE_REVISIED_BATTLE
# )
