cmake_minimum_required (VERSION 3.14)
project(palgame C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

if(UNIX)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY             ${CMAKE_BINARY_DIR})
else()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG       ${CMAKE_BINARY_DIR})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE     ${CMAKE_BINARY_DIR})
endif()

set (GLFW_DIR           external/glfw)
set (GAME_DIR           src)
set (DRIVER_DIR         driver)
set (RESOURCE_PATH      ${CMAKE_BINARY_DIR}/resource)

file(GLOB GAME_SOURCES
    ${GAME_DIR}/*.c
    ${GAME_DIR}/adplug/*.c
    ${GAME_DIR}/sound/*.c)

set(SDL_SHARED              OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST                OFF CACHE BOOL "Build the SDL2_test library" FORCE)
set(SDL2_DISABLE_SDL2MAIN   ON  CACHE BOOL "Disable building/installation of SDL2main" FORCE)
add_subdirectory(external/SDL)
set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

add_definitions(-DRESOURCE_PATH="${RESOURCE_PATH}")
add_definitions(-DPAL_HAS_JOYSTICKS)

include_directories(external)
include_directories(${DRIVER_DIR})

add_executable(${PROJECT_NAME}_sdl
    ${DRIVER_DIR}/SDL.c
    ${DRIVER_DIR}/Audio_SDL.c
    ${DRIVER_DIR}/Event_SDL.c
    ${DRIVER_DIR}/Video_SDL.c
    ${DRIVER_DIR}/mini_glloader.c
    ${DRIVER_DIR}/video_glsl.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_glfw
    ${DRIVER_DIR}/GLFW.c
    ${DRIVER_DIR}/Audio_miniaudio.c
    ${DRIVER_DIR}/Event_GLFW.c
    ${DRIVER_DIR}/Video_GLFW.c
    ${DRIVER_DIR}/video_glsl.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_dummy
    ${DRIVER_DIR}/driver.c
    ${GAME_SOURCES}
)

target_include_directories(${PROJECT_NAME}_sdl
    PUBLIC
        ${SDL_INCLUDE_DIR}
)

target_include_directories(${PROJECT_NAME}_glfw
    PUBLIC
        ${GLFW_DIR}
        ${GLFW_DIR}/deps
)

target_compile_definitions(${PROJECT_NAME}_sdl  PRIVATE _GLLOADER_SDL)
target_compile_definitions(${PROJECT_NAME}_glfw PRIVATE _GLLOADER_GLFW)

target_link_libraries(${PROJECT_NAME}_sdl
    ${OPENGL_LIBRARY}
    SDL2::SDL2
)

target_link_libraries(${PROJECT_NAME}_glfw
    ${OPENGL_LIBRARY}
    glfw
)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME}_dummy m)
endif()


file(GLOB GAMEDATA
    ${CMAKE_SOURCE_DIR}/Pal98rqptw/*.mkf
    ${CMAKE_SOURCE_DIR}/Pal98rqptw/*.msg
    ${CMAKE_SOURCE_DIR}/Pal98rqptw/*.dat)
file(GLOB CACHEDATA ${CMAKE_SOURCE_DIR}/cache_data/*.bin)
file(COPY ${GAMEDATA}                       DESTINATION ${RESOURCE_PATH})
file(COPY ${CACHEDATA}                      DESTINATION ${RESOURCE_PATH})
file(COPY ${CMAKE_SOURCE_DIR}/shaders/      DESTINATION ${CMAKE_BINARY_DIR}/shaders)
