cmake_minimum_required (VERSION 3.13)
project(sdlpal_win32 C CXX)

set(CMAKE_CXX_STANDARD 14)

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

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (SDL2_DIR           external/SDL)
set (BINIO_SOURCE_DIR   external/libbinio/src)
set (BINIO_INCLUDE_DIR  external/libbinio/contrib/vs2015/libbinio)
set (NUKED_DIR          external/nuked)
set (ADPLUG_DIR         adplug)
set (SDLPAL_DIR         src)
set (DRIVER_DIR         driver)
set (RESOURCE_PATH      ${CMAKE_BINARY_DIR}/resource/)

file(GLOB_RECURSE SDLPAL_SOURCES    ${SDLPAL_DIR}/*.c ${SDLPAL_DIR}/*.cpp)
file(GLOB NUKED_SOURCES             ${NUKED_DIR}/*.c)
file(GLOB BINIO_SOURCES             ${BINIO_SOURCE_DIR}/*.cpp)
file(GLOB ADPLUG_SOURCES            ${ADPLUG_DIR}/*.cpp)

set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST   OFF CACHE BOOL "Build the SDL2_test library" FORCE)
add_subdirectory(external/SDL)

add_definitions(-DSOURCE_DIR="${CMAKE_BINARY_DIR}")
add_definitions(-DRESOURCE_PATH="${RESOURCE_PATH}")

add_executable(${PROJECT_NAME}
    ${DRIVER_DIR}/SDL.c
    ${DRIVER_DIR}/Audio_SDL.c
    # ${DRIVER_DIR}/Audio_miniaudio.c
    ${DRIVER_DIR}/mini_glloader.c
    ${DRIVER_DIR}/video_glsl.c
    ${SDLPAL_SOURCES}
    ${NUKED_SOURCES}
    ${BINIO_SOURCES}
    ${ADPLUG_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${SDL2_DIR}/include()
        ${CMAKE_CURRENT_SOURCE_DIR}
        external/
        ${BINIO_SOURCE_DIR}
        ${BINIO_INCLUDE_DIR}
        ${SDLPAL_DIR}
        ${DRIVER_DIR}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        SDL2-static
        SDL2main
    )
else()
    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        OpenGL::OpenGL OpenGL::EGL 
        SDL2-static
        SDL2main
    )
endif()

if (MSVC)
    file(GLOB PAL_GAMEDATA
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.mkf
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.msg
        ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw/*.dat)
    file(COPY ${PAL_GAMEDATA}                                   DESTINATION ${RESOURCE_PATH})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/              DESTINATION ${CMAKE_BINARY_DIR}/shaders)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cptbl_big5.dat        DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font.dat      DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font_size.dat DESTINATION ${CMAKE_BINARY_DIR})
else()
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/Pal98rqptw               ${CMAKE_BINARY_DIR}/resource
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/shaders                  ${CMAKE_BINARY_DIR}/shaders
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/cptbl_big5.dat           ${CMAKE_BINARY_DIR}/cptbl_big5.dat
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font.dat         ${CMAKE_BINARY_DIR}/unicode_font.dat
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/unicode_font_size.dat    ${CMAKE_BINARY_DIR}/unicode_font_size.dat
    )
endif()
