add_library(SDL2main
    ${SDL2_DIR}/src/main/windows/SDL_windows_main.c
)

target_include_directories(SDL2main
    PRIVATE
        ${SDL2_DIR}/include
)