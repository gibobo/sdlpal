set(SDLPAL_SOURCE_FILES
    ${SDLPAL_DIR}/audio.c
    ${SDLPAL_DIR}/battle.c
    ${SDLPAL_DIR}/ending.c
    ${SDLPAL_DIR}/fight.c
    ${SDLPAL_DIR}/font.c
    ${SDLPAL_DIR}/game.c
    ${SDLPAL_DIR}/global.c
    ${SDLPAL_DIR}/glslp.c
    ${SDLPAL_DIR}/input.c
    ${SDLPAL_DIR}/itemmenu.c
    ${SDLPAL_DIR}/magicmenu.c
    ${SDLPAL_DIR}/map.c
    ${SDLPAL_DIR}/mini_glloader.c
    ${SDLPAL_DIR}/palcfg.c
    ${SDLPAL_DIR}/palcommon.c
    ${SDLPAL_DIR}/palette.c
    ${SDLPAL_DIR}/play.c
    ${SDLPAL_DIR}/res.c
    ${SDLPAL_DIR}/resampler.c
    ${SDLPAL_DIR}/rixplay.cpp
    ${SDLPAL_DIR}/rngplay.c
    ${SDLPAL_DIR}/scene.c
    ${SDLPAL_DIR}/script.c
    ${SDLPAL_DIR}/sound.c
    ${SDLPAL_DIR}/text.c
    ${SDLPAL_DIR}/ui.c
    ${SDLPAL_DIR}/uibattle.c
    ${SDLPAL_DIR}/uigame.c
    ${SDLPAL_DIR}/util.c
    ${SDLPAL_DIR}/video.c
    ${SDLPAL_DIR}/video_glsl.c
    ${SDLPAL_DIR}/yj1.c
)

add_library(sdlpal
    STATIC
        ${SDLPAL_SOURCE_FILES}
)

target_include_directories(sdlpal
    PRIVATE
        # ${CMAKE_SOURCE_DIR}
        win32
        ${SDLPAL_DIR}
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${BINIO_INCLUDE_DIR}
)

# target_compile_definitions(sdlpal
#     PRIVATE
#     PAL_CLASSIC
#     ENABLE_REVISIED_BATTLE
# )
