add_library(SDL2main
    ${SDL2_SOURCE_DIR}/main/windows/SDL_windows_main.c
)

target_include_directories(SDL2main
    PRIVATE
        ${SDL2_INCLUDE_DIR}
)