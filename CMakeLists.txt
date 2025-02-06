cmake_minimum_required (VERSION 3.14)
project(palgame C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(EGL)
# find_package(GLES)
# find_package (GLUT)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG       ${CMAKE_BINARY_DIR})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE     ${CMAKE_BINARY_DIR})
endif()


include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.12
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/SDL
)
FetchContent_Declare(libbinio
    GIT_REPOSITORY https://github.com/adplug/libbinio.git
    GIT_TAG libbinio-1.5
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/libbinio
)
FetchContent_Declare(nuked
    GIT_REPOSITORY https://github.com/nukeykt/Nuked-OPL3.git
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/nuked
)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/glfw
)
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/miniaudio
)

set(SDL_SHARED              OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST                OFF CACHE BOOL "Build the SDL2_test library" FORCE)
set(SDL2_DISABLE_SDL2MAIN   ON  CACHE BOOL "Disable building/installation of SDL2main" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL glfw libbinio nuked miniaudio)

set (BINIO_SOURCE_DIR   ${libbinio_SOURCE_DIR}/src)
set (BINIO_INCLUDE_DIR  ${libbinio_SOURCE_DIR}/contrib/vs2015/libbinio)
set (ADPLUG_DIR         adplug)
set (GAME_DIR           src)
set (DRIVER_DIR         driver)
set (RESOURCE_PATH      ${CMAKE_BINARY_DIR}/resource/)

file(GLOB_RECURSE GAME_SOURCES      ${GAME_DIR}/*.c ${GAME_DIR}/*.cpp)
file(GLOB NUKED_SOURCES             ${nuked_SOURCE_DIR}/*.c)
file(GLOB BINIO_SOURCES             ${BINIO_SOURCE_DIR}/*.cpp)
file(GLOB ADPLUG_SOURCES            ${ADPLUG_DIR}/*.cpp)

add_definitions(-DRESOURCE_PATH="${RESOURCE_PATH}")
add_definitions(-DPAL_HAS_JOYSTICKS)

include_directories(external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${BINIO_SOURCE_DIR})
include_directories(${BINIO_INCLUDE_DIR})
include_directories(${miniaudio_SOURCE_DIR})
include_directories(${glfw_SOURCE_DIR}/deps)
include_directories(${GAME_DIR})
include_directories(${DRIVER_DIR})

add_executable(${PROJECT_NAME}_sdl
    ${DRIVER_DIR}/SDL.c
    # ${DRIVER_DIR}/Audio_SDL.c
    ${DRIVER_DIR}/Event_SDL.c
    ${DRIVER_DIR}/Video_SDL.c
    ${DRIVER_DIR}/Audio_miniaudio.c
    ${DRIVER_DIR}/mini_glloader.c
    ${DRIVER_DIR}/video_glsl.c
    ${GAME_SOURCES}
    ${NUKED_SOURCES}
    ${BINIO_SOURCES}
    ${ADPLUG_SOURCES}
)

add_executable(${PROJECT_NAME}_glfw
    ${DRIVER_DIR}/GLFW.c
    ${DRIVER_DIR}/Audio_miniaudio.c
    ${DRIVER_DIR}/Event_GLFW.c
    ${DRIVER_DIR}/Video_GLFW.c
    # ${DRIVER_DIR}/mini_glloader.c
    # ${DRIVER_DIR}/video_glsl.c
    ${GAME_SOURCES}
    ${NUKED_SOURCES}
    ${BINIO_SOURCES}
    ${ADPLUG_SOURCES}
)

target_include_directories(${PROJECT_NAME}_sdl
    PUBLIC
        ${SDL_INCLUDE_DIR}
)

target_include_directories(${PROJECT_NAME}_glfw
    PUBLIC
        ${glfw_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}_sdl
    ${OPENGL_LIBRARY}
    SDL2::SDL2
)

target_link_libraries(${PROJECT_NAME}_glfw
    ${OPENGL_LIBRARY}
    glfw
)

if (MSVC)
    file(GLOB GAME_GAMEDATA
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.mkf
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.msg
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.dat)
    file(COPY ${GAME_GAMEDATA}                                  DESTINATION ${RESOURCE_PATH})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/              DESTINATION ${CMAKE_BINARY_DIR}/shaders)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cptbl_big5.dat        DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font.dat      DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font_size.dat DESTINATION ${CMAKE_BINARY_DIR})
else()
    add_custom_command(TARGET ALL
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw               ${CMAKE_BINARY_DIR}/resource
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/shaders                  ${CMAKE_BINARY_DIR}/shaders
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/cptbl_big5.dat           ${CMAKE_BINARY_DIR}/cptbl_big5.dat
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font.dat         ${CMAKE_BINARY_DIR}/unicode_font.dat
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font_size.dat    ${CMAKE_BINARY_DIR}/unicode_font_size.dat
    )
endif()
