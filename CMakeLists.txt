# CMakeList.txt: 最上層的 CMake 專案檔，執行全域設定
# 並在這裡包含子專案。
#
cmake_minimum_required (VERSION 3.8)


# 如果支援，則為 MSVC 編譯器啟用熱重新載入。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(sdlpal_win32)

# CMakeList.txt: cmake 的 CMake 專案，在此包含來源及定義
# 專案專屬邏輯。
#
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set (SDL_MIXER_SOURCE_DIR   external/SDL_mixer/src/codecs)
set (SDL_MIXER_INCLUDE_DIR  external/SDL_mixer/include)

include(cmake/SDL2.cmake)
include(cmake/SDL2main.cmake)
include(cmake/libbinio.cmake)
include(cmake/adplug.cmake)
include(cmake/libmad.cmake)
include(cmake/libogg.cmake)
include(cmake/vorbis.cmake)
include(cmake/opus.cmake)
include(cmake/opusfile.cmake)
include(cmake/timidity.cmake)
include(cmake/native_midi.cmake)
include(cmake/sdlpal.cmake)

add_executable (${PROJECT_NAME} win32/win32.cpp)
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
  vorbis
  libogg
  timidity
  opus
  opusfile
  libmad
  sdlpal
  libbinio
  adplug
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    win32
    ${SDLPAL_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${ADPLUG_DIR}
    )
