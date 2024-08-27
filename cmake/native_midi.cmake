add_library(native_midi
    ${NATIVE_MIDI_DIR}/native_midi_common.c
    ${NATIVE_MIDI_DIR}/native_midi_win32.c
)

target_include_directories(native_midi
    PRIVATE
        ${SDL2_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_DIR}
        ${NATIVE_MIDI_DIR}
)
