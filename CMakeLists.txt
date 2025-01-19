# CMakeList.txt: 最上層的 CMake 專案檔，執行全域設定
# 並在這裡包含子專案。
#
cmake_minimum_required (VERSION 3.8)
project(sdlpal_win32 C CXX)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(EGL)
# find_package(GLES)
find_package (GLUT)

add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()
#
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (SDL2_DIR               external/SDL)
set (BINIO_SOURCE_DIR       external/libbinio/src)
set (BINIO_INCLUDE_DIR      external/libbinio/contrib/vs2015/libbinio)
set (NUKED_DIR              external/nuked)
set (ADPLUG_DIR             adplug)
set (SDLPAL_DIR             src)

file(GLOB_RECURSE SDLPAL_SOURCES    ${SDLPAL_DIR}/*.c ${SDLPAL_DIR}/*.cpp)
file(GLOB_RECURSE SDLPAL_HEADERS	${SDLPAL_DIR}/*.h)

set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST   OFF CACHE BOOL "Build the SDL2_test library" FORCE)
add_subdirectory(external/SDL)

# add_subdirectory(external/libbinio)

# if(WIN32)
# include(cmake/SDL2.cmake)
# include(cmake/SDL2main.cmake)
# endif()

include(cmake/adplug.cmake)

add_executable(${PROJECT_NAME}
    ${SDLPAL_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${SDL2_DIR}/include()
        ${BINIO_INCLUDE_DIR}
        ${ADPLUG_DIR}
        ${SDLPAL_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        SDL2-static
        SDL2main
        adplug
    )
else()
    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        OpenGL::OpenGL OpenGL::EGL 
        SDL2-static
        SDL2main
        adplug
    )
endif()