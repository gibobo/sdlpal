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

if(WIN32)
    set(PLATFORM_DIR win32)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()
else()
    set(PLATFORM_DIR unix)
endif()
#
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (SDL2_DIR               external/SDL)
set (BINIO_SOURCE_DIR       external/libbinio/src)
set (BINIO_INCLUDE_DIR      external/libbinio/contrib/vs2015/libbinio)
set (NUKED_DIR              external/nuked)
set (ADPLUG_DIR             adplug)
set (SDLPAL_DIR             sdlpal/)

add_subdirectory(external/SDL)
# add_subdirectory(external/libbinio)

# if(WIN32)
# include(cmake/SDL2.cmake)
# include(cmake/SDL2main.cmake)
# endif()

include(cmake/adplug.cmake)
include(cmake/sdlpal.cmake)

if(WIN32)
    add_executable (${PROJECT_NAME}
        ${SDLPAL_DIR}/main.c
        win32/win32.cpp
    )

    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARY}
        SDL2-static
        SDL2main
        adplug
        sdlpal
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${PLATFORM_DIR}
            ${SDL2_DIR}/include
            ${ADPLUG_DIR}
            ${SDLPAL_DIR}
    )
else()
    # find_package(FLTK)
    add_executable (${PROJECT_NAME}
        ${ADPLUG_FILES}
        ${SDLPAL_DIR}/main.c
        unix/unix.cpp
    )

    # sudo apt-get install libglfw3-dev libgles2-mesa-dev
    target_link_libraries(${PROJECT_NAME}
        # ${FLTK_LIBRARIES}
        ${OPENGL_LIBRARY}
        OpenGL::OpenGL OpenGL::EGL 
        SDL2-static
        SDL2main
        sdlpal
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${PLATFORM_DIR}
            ${SDL2_DIR}/include
            ${ADPLUG_DIR}
            ${SDLPAL_DIR}
            ${BINIO_SOURCE_DIR}
            ${BINIO_INCLUDE_DIR}
            ${NUKED_DIR}
            # ${FLTK_INCLUDE_DIR}
    )

endif()