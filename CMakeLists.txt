# CMakeList.txt: 最上層的 CMake 專案檔，執行全域設定
# 並在這裡包含子專案。
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 14)

# find_package(Threads REQUIRED)

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

set (SDL2_SOURCE_DIR        external/SDL)
set (BINIO_SOURCE_DIR       external/libbinio/src)
set (BINIO_INCLUDE_DIR      external/libbinio/contrib/vs2015/libbinio)
set (NUKED_DIR              external/nuked)
set (ADPLUG_DIR             adplug)
set (SDLPAL_DIR             sdlpal/)

if(WIN32)
include(cmake/SDL2.cmake)
include(cmake/SDL2main.cmake)
endif()

include(cmake/adplug.cmake)
include(cmake/sdlpal.cmake)

if(WIN32)

add_executable (${PROJECT_NAME}
    ${SDLPAL_DIR}/main.c
    win32/win32.cpp
)

target_link_libraries(${PROJECT_NAME}
    opengl32
    imm32
    winmm
    setupapi
    version
    SDL2
    SDL2main
    adplug
    sdlpal
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        win32
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${SDLPAL_DIR}
)
else()
find_package(OpenGL REQUIRED)

add_executable (${PROJECT_NAME}
    unix/unix.cpp
    ${SDLPAL_DIR}/main.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        external/SDL_mixer/src/codecs
        ${SDL2_SOURCE_DIR}/include
        ${ADPLUG_DIR}
        ${SDLPAL_DIR}
        ${OPENGL_INCLUDE_DIR}
        unix
)

# sudo apt-get install libglfw3-dev libgles2-mesa-dev
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    OpenGL::GLU
    OpenGL::EGL
    glfw
    fltk
    SDL2
    sdlpal
    adplug
    ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
)

endif()