set (SDLPAL_SOURCE_DIR  sdlpal/source)
set (SDLPAL_INCLUDE_DIR sdlpal/include)

set(SDLPAL_SOURCE_FILES
  ${SDLPAL_SOURCE_DIR}/audio.c
  ${SDLPAL_SOURCE_DIR}/aviplay.c
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
  ${SDLPAL_SOURCE_DIR}/main.c
  ${SDLPAL_SOURCE_DIR}/map.c
  ${SDLPAL_SOURCE_DIR}/midi.c
  ${SDLPAL_SOURCE_DIR}/midi_timidity.c
  ${SDLPAL_SOURCE_DIR}/midi_tsf.c
  ${SDLPAL_SOURCE_DIR}/mini_glloader.c
  ${SDLPAL_SOURCE_DIR}/mp3play.c
  ${SDLPAL_SOURCE_DIR}/music_mad.c
  ${SDLPAL_SOURCE_DIR}/oggplay.c
  ${SDLPAL_SOURCE_DIR}/opusplay.c
  ${SDLPAL_SOURCE_DIR}/overlay.c
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

add_library(sdlpal ${SDLPAL_SOURCE_FILES})

target_include_directories(sdlpal
    PRIVATE
        win32
        ${SDLPAL_INCLUDE_DIR}
        ${SDL2_INCLUDE_DIR}
        ${SDL_MIXER_SOURCE_DIR}
        ${TIMIDITY_DIR}
        ${VORBIS_INCLUDE_DIR}
        ${LIBOGG_INCLUDE_DIR}
        ${OPUS_INCLUDE_DIR}
        ${OPUSFILE_INCLUDE_DIR}
        ${LIBMAD_DIR}
        ${ADPLUG_DIR}
        ${LIBBINIO_SOURCE_DIR}
        ${LIBBINIO_INCLUDE_DIR}
)

target_compile_definitions(sdlpal
    PRIVATE
        PAL_HAS_PLATFORM_SPECIFIC_UTILS
        _CONSOLE
)
