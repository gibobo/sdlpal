# CMakeList.txt: 最上層的 CMake 專案檔，執行全域設定
# 並在這裡包含子專案。
#
cmake_minimum_required (VERSION 3.8)


# 如果支援，則為 MSVC 編譯器啟用熱重新載入。
if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM Win32)
    if (POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()
    target_compile_definitions(${PROJECT_NAME} "-D_CRT_SECURE_NO_DEPRECATE")
    target_compile_definitions(${PROJECT_NAME} INTERFACE "-D_CRT_NONSTDC_NO_DEPRECATE")
    target_compile_definitions(${PROJECT_NAME} INTERFACE "-D_CRT_SECURE_NO_WARNINGS")
endif()

project(sdlpal_win32)

# CMakeList.txt: cmake 的 CMake 專案，在此包含來源及定義
# 專案專屬邏輯。
#
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(OPUS_BUILD_SHARED_LIBRARY OFF)
set(OPUS_BUILD_TESTING OFF)
set(OPUS_BUILD_PROGRAMS OFF)
set(OP_DISABLE_HTTP ON)
set(OP_DISABLE_EXAMPLES ON)
set(OP_DISABLE_DOCS ON)

# add_subdirectory(external/SDL)
# add_subdirectory(external/SDL_mixer)
add_subdirectory(external/SDL_mixer/external/ogg)
add_subdirectory(external/SDL_mixer/external/vorbis)
add_subdirectory(external/SDL_mixer/external/opus)
add_subdirectory(external/SDL_mixer/external/opusfile)

set (SDL2_SOURCE_DIR        external/SDL)
set (SDL_MIXER_INCLUDE_DIR  external/SDL_mixer/include)
set (SDL_MIXER_SOURCE_DIR   external/SDL_mixer/src/codecs)
set (NATIVE_MIDI_DIR        external/SDL_mixer/src/codecs/native_midi)
set (TIMIDITY_DIR           external/SDL_mixer/src/codecs/timidity)
set (OGG_DIR                external/SDL_mixer/external/ogg)
# set (OPUS_SOURCE_DIR        external/SDL_mixer/external/opus)
set (OPUS_INCLUDE_DIR       external/SDL_mixer/external/opus/include)
set (OPUSFILE_SOURCE_DIR    external/SDL_mixer/external/opusfile/src)
set (OPUSFILE_INCLUDE_DIR   external/SDL_mixer/external/opusfile/include)
set (VORBIS_SOURCE_DIR      external/SDL_mixer/external/vorbis/lib)
set (VORBIS_INCLUDE_DIR     external/SDL_mixer/external/vorbis/include)

set (BINIO_SOURCE_DIR       external/libbinio/src)
set (BINIO_INCLUDE_DIR      external/libbinio/contrib/vs2015/libbinio)
set (MAD_DIR                external/libmad)
set (NUKED_DIR              external/nuked)
set (ADPLUG_DIR             adplug)
set (SDLPAL_SOURCE_DIR      sdlpal/source)
set (SDLPAL_INCLUDE_DIR     sdlpal/include)

if(WIN32)
include(cmake/SDL2.cmake)
include(cmake/SDL2main.cmake)
else()
endif()

# include(cmake/opus.cmake)
# include(cmake/opusfile.cmake)
include(cmake/timidity.cmake)
include(cmake/native_midi.cmake)

include(cmake/mad.cmake)
include(cmake/adplug.cmake)
include(cmake/sdlpal.cmake)

if(WIN32)

add_executable (${PROJECT_NAME}
    win32/win32.cpp
    ${SDLPAL_SOURCE_DIR}/main.c
)

target_link_libraries(${PROJECT_NAME}
    opengl32
    imm32
    winmm
    comctl32
    setupapi
    version
    SDL2
    SDL2main
    native_midi
    timidity
    vorbis
    Ogg::ogg
    Opus::opus
    opusfile
    mad
    adplug
    sdlpal
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        external/SDL_mixer/src/codecs
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${SDLPAL_INCLUDE_DIR}
        win32
)
else()
endif()