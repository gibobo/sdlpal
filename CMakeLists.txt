cmake_minimum_required(VERSION 3.14)

# Project definition
project(palgame C CXX)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output directories
if(UNIX)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
endif()

# External and source directories
set(GLFW_DIR        external/glfw)
set(WEBSERVER_DIR   external/mongoose)
set(GAME_DIR        src)
set(DRIVER_DIR      driver)
set(RESOURCE_PATH   ${CMAKE_BINARY_DIR}/resource)
set(CACHES_PATH     ${CMAKE_BINARY_DIR}/caches)

# Source files
file(GLOB GAME_SOURCES
    ${GAME_DIR}/*.c
    ${GAME_DIR}/adplug/*.c
    ${GAME_DIR}/sound/*.c
)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# SDL2 configuration
set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build the SDL2_test library" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "Disable building/installation of SDL2main" FORCE)
add_subdirectory(external/SDL)

# GLFW configuration
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# libcaca
add_subdirectory(external/libcaca)

# Definitions
add_definitions(-DRESOURCE_PATH="${RESOURCE_PATH}")
add_definitions(-DCACHES_PATH="${CACHES_PATH}")
add_definitions(-DPAL_HAS_JOYSTICKS)
# Include directories
include_directories(
    external
    ${DRIVER_DIR}
)

# Link libraries
if(WIN32)
    link_libraries(${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
else()
    link_libraries(${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} m)
endif()

# Executable targets
add_executable(${PROJECT_NAME}_sdl
    ${DRIVER_DIR}/DrvIf_SDL.c
    ${DRIVER_DIR}/Audio_SDL.c
    ${DRIVER_DIR}/Event_SDL.c
    ${DRIVER_DIR}/Video_SDL.c
    ${DRIVER_DIR}/utils/mini_glloader.c
    ${DRIVER_DIR}/utils/video_glsl.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_glfw
    ${DRIVER_DIR}/DrvIf_GLFW.c
    ${DRIVER_DIR}/Audio_miniaudio.c
    ${DRIVER_DIR}/Event_GLFW.c
    ${DRIVER_DIR}/Video_GLFW.c
    ${DRIVER_DIR}/utils/video_glsl.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_caca
    ${DRIVER_DIR}/DrvIf.c
    ${DRIVER_DIR}/Audio_Dummy.c
    ${DRIVER_DIR}/Event_CACA.c
    ${DRIVER_DIR}/Video_CACA.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_dummy
    ${DRIVER_DIR}/DrvIf.c
    ${DRIVER_DIR}/Audio_Dummy.c
    ${DRIVER_DIR}/Event_Dummy.c
    ${DRIVER_DIR}/Video_Dummy.c
    ${GAME_SOURCES}
)

add_executable(${PROJECT_NAME}_webserver
    ${DRIVER_DIR}/DrvIf.c
    ${DRIVER_DIR}/Audio_WEB.c
    ${DRIVER_DIR}/Event_WEB.c
    ${DRIVER_DIR}/Video_WEB.c
    ${WEBSERVER_DIR}/mongoose.c
    ${GAME_SOURCES}
)

# Target include directories
target_include_directories(${PROJECT_NAME}_sdl PUBLIC ${SDL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME}_glfw PUBLIC ${GLFW_DIR} ${GLFW_DIR}/deps)
target_include_directories(${PROJECT_NAME}_caca PUBLIC ${CACA_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME}_webserver PUBLIC ${WEBSERVER_DIR})

# Target compile definitions
target_compile_definitions(${PROJECT_NAME}_sdl PRIVATE _GLLOADER_SDL)
target_compile_definitions(${PROJECT_NAME}_glfw PRIVATE _GLLOADER_GLFW)

# Target link libraries
target_link_libraries(${PROJECT_NAME}_sdl   ${OPENGL_LIBRARY} SDL2::SDL2)
target_link_libraries(${PROJECT_NAME}_glfw  ${OPENGL_LIBRARY} glfw)
target_link_libraries(${PROJECT_NAME}_caca  ${CACA_LIBRARYS})
if(WIN32)
    target_link_libraries(${PROJECT_NAME}_webserver ws2_32)
endif()

# Symlink creation
if(WIN32)
    set(MKLINK_SCRIPT "${CMAKE_BINARY_DIR}/create_symlinks.bat")
    file(WRITE "${MKLINK_SCRIPT}" 
        "mklink \"${CMAKE_BINARY_DIR}/index.html\" \"${CMAKE_SOURCE_DIR}/index.html\"\n"
        "mklink /D \"${RESOURCE_PATH}\" \"${CMAKE_SOURCE_DIR}/Pal98rqptw\"\n"
        "mklink /D \"${CACHES_PATH}\" \"${CMAKE_SOURCE_DIR}/cache_data\"\n"
        "mklink /D \"${CMAKE_BINARY_DIR}/shaders\" \"${CMAKE_SOURCE_DIR}/shaders\"\n"
    )
    execute_process(
        COMMAND powershell -Command "Start-Process cmd -ArgumentList '/c \"${MKLINK_SCRIPT}\"' -Verb runAs"
        RESULT_VARIABLE elevate_result
    )
else()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/index.html" "${CMAKE_BINARY_DIR}/index.html")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/shaders"    "${CMAKE_BINARY_DIR}/shaders")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/Pal98rqptw" "${RESOURCE_PATH}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/cache_data" "${CACHES_PATH}")
endif()