add_library(mad
    STATIC
        ${MAD_DIR}/bit.c
        ${MAD_DIR}/decoder.c
        ${MAD_DIR}/fixed.c
        ${MAD_DIR}/frame.c
        ${MAD_DIR}/huffman.c
        ${MAD_DIR}/layer12.c
        ${MAD_DIR}/layer3.c
        ${MAD_DIR}/stream.c
        ${MAD_DIR}/synth.c
        ${MAD_DIR}/timer.c
)

target_include_directories(mad
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(mad
    PRIVATE
        HAVE_CONFIG_H
        FPM_DEFAULT
)
