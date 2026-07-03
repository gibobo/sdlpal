cmake_minimum_required(VERSION 3.14)

# Project definition
project(palgame C CXX)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output directories
if(UNIX)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
else()
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
    ${GAME_DIR}/adplug/dosbox/*.c
    ${GAME_DIR}/adplug/nuked/*.c
    ${GAME_DIR}/sound/*.c
)
# ESP32 backends live under src/driver_esp32/ so arduino-cli compiles them recursively;
# keep them out of the desktop build. No-op with the non-recursive globs above, but
# guards against a future switch to GLOB_RECURSE.
list(FILTER GAME_SOURCES EXCLUDE REGEX "/driver_esp32/")

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB)

# SDL2 configuration
set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build the SDL2_test library" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "Disable building/installation of SDL2main" FORCE)

# Save current flags and disable warnings for external libraries
set(SAVED_C_FLAGS "${CMAKE_C_FLAGS}")
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")  # Suppress all warnings for external code
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")  # Suppress all warnings for external code
endif()

add_subdirectory(external/SDL)

# GLFW configuration
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# libcaca
add_subdirectory(external/libcaca)

# Restore original flags for our project code
set(CMAKE_C_FLAGS "${SAVED_C_FLAGS}")

# Compiler warning flags function
function(set_project_warnings target_name)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(PROJECT_WARNING_FLAGS
            -Wall                    # Enable most warnings
            -Wextra                  # Enable extra warnings
            -Wconversion             # Warn about type conversions that may lose data
            -Wsign-conversion        # Warn about signed/unsigned conversions
            -Wcast-align             # Warn about pointer casts that increase alignment
            -Wformat=2               # Warn about format string issues
            -Wformat-security        # Warn about security issues in format strings
            -Wdouble-promotion       # Warn about float to double promotions
            -Wundef                  # Warn about undefined macros
            -Wstrict-prototypes      # Warn about function declarations without prototypes (C only)
            -Wmissing-prototypes     # Warn about missing function prototypes (C only)
            -Wold-style-definition   # Warn about old-style function definitions (C only)
        )
        
        # Add warning flags that are available in newer versions
        include(CheckCCompilerFlag)
        
        # Check for unsigned conversion warning (GCC 7.0+)
        check_c_compiler_flag(-Wunsigned-conversion HAS_UNSIGNED_CONVERSION_WARNING)
        if(HAS_UNSIGNED_CONVERSION_WARNING)
            list(APPEND PROJECT_WARNING_FLAGS -Wunsigned-conversion)
        endif()
        
        check_c_compiler_flag(-Wlogical-op HAS_LOGICAL_OP_WARNING)
        if(HAS_LOGICAL_OP_WARNING)
            list(APPEND PROJECT_WARNING_FLAGS -Wlogical-op)
        endif()
        
        check_c_compiler_flag(-Wduplicated-cond HAS_DUPLICATED_COND_WARNING)
        if(HAS_DUPLICATED_COND_WARNING)
            list(APPEND PROJECT_WARNING_FLAGS -Wduplicated-cond)
        endif()
        
        check_c_compiler_flag(-Wduplicated-branches HAS_DUPLICATED_BRANCHES_WARNING)
        if(HAS_DUPLICATED_BRANCHES_WARNING)
            list(APPEND PROJECT_WARNING_FLAGS -Wduplicated-branches)
        endif()
        
        # Apply warning flags to specific target
        target_compile_options(${target_name} PRIVATE ${PROJECT_WARNING_FLAGS})
        
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        # MSVC warning flags
        target_compile_options(${target_name} PRIVATE
            /W4          # Enable level 4 warnings
            /we4013      # Treat undefined function as error
            /we4024      # Different types for formal and actual parameter
            /we4047      # Different levels of indirection
            /we4133      # Incompatible types
        )
        target_compile_definitions(${target_name} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE
            _SCL_SECURE_NO_WARNINGS
        )
    endif()
endfunction()

# Definitions
add_definitions(-DRESOURCE_PATH="${RESOURCE_PATH}")
add_definitions(-DCACHES_PATH="${CACHES_PATH}")
add_definitions(-DPAL_HAS_JOYSTICKS)

# Desktop opt-in: stream pre-rendered PCM music (see src/pcmmus.c) instead of the
# live RIX/OPL3 synthesizer, so the same PCM files/player used on ESP32 can be
# auditioned and debugged on the desktop build. Off by default (desktop keeps RIX);
# turn on with -DPAL_PRERENDERED_MUSIC=ON. On ESP32 this path is always on.
option(PAL_PRERENDERED_MUSIC "Desktop: stream pre-rendered PCM music instead of live RIX/OPL3" OFF)
if(PAL_PRERENDERED_MUSIC)
    add_definitions(-DPAL_PRERENDERED_MUSIC=1)
endif()
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

# --- Offline music pre-render tool (standalone executable) ---
# Renders every RIX track to headered PCM using the exact Nuked-OPL3/RIX synthesis,
# resampling to any target rate with the engine's resampler. Headless (Dummy driver)
# and has its OWN main() (tools/musrender.c). src/main.c is compiled with PAL_NO_MAIN
# so its game main() is excluded (avoiding a duplicate main) while its other
# functions (PAL_Shutdown, etc.) remain available to the linked engine.
add_executable(palmusrender
    ${DRIVER_DIR}/DrvIf.c
    ${DRIVER_DIR}/Audio_Dummy.c
    ${DRIVER_DIR}/Event_Dummy.c
    ${DRIVER_DIR}/Video_Dummy.c
    ${GAME_SOURCES}
    tools/musrender.c
)
target_include_directories(palmusrender PRIVATE ${GAME_DIR} ${DRIVER_DIR})
# Override the global RESOURCE_PATH/CACHES_PATH (which point at the game's build/resource
# symlink) so the tool reads game data from the CWD it chdirs into (its --data argument).
target_compile_options(palmusrender PRIVATE -URESOURCE_PATH -UCACHES_PATH)
target_compile_definitions(palmusrender PRIVATE PAL_NO_MAIN RESOURCE_PATH="." CACHES_PATH=".")

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

# Apply project-specific warning flags to all our executables
set_project_warnings(${PROJECT_NAME}_sdl)
set_project_warnings(${PROJECT_NAME}_glfw)
set_project_warnings(${PROJECT_NAME}_caca)
set_project_warnings(${PROJECT_NAME}_dummy)
set_project_warnings(${PROJECT_NAME}_webserver)

# Symlink creation
if(WIN32)
    # set(MKLINK_SCRIPT "${CMAKE_BINARY_DIR}/create_symlinks.bat")
    # file(WRITE "${MKLINK_SCRIPT}" 
    #     "mklink \"${CMAKE_BINARY_DIR}/index.html\" \"${CMAKE_SOURCE_DIR}/index.html\"\n"
    #     "mklink /D \"${RESOURCE_PATH}\" \"${CMAKE_SOURCE_DIR}/Pal98rqptw\"\n"
    #     "mklink /D \"${CACHES_PATH}\" \"${CMAKE_SOURCE_DIR}/cache_data\"\n"
    #     "mklink /D \"${CMAKE_BINARY_DIR}/shaders\" \"${CMAKE_SOURCE_DIR}/shaders\"\n"
    # )
    # execute_process(
    #     COMMAND powershell -Command "Start-Process cmd -ArgumentList '/c \"${MKLINK_SCRIPT}\"' -Verb runAs"
    #     RESULT_VARIABLE elevate_result
    # )
else()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/index.html" "${CMAKE_BINARY_DIR}/index.html")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/shaders"    "${CMAKE_BINARY_DIR}/shaders")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/Pal98rqptw" "${RESOURCE_PATH}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_SOURCE_DIR}/cache_data" "${CACHES_PATH}")
endif()