add_library(ogg
    ${OGG_DIR}/src/bitwise.c
    ${OGG_DIR}/src/framing.c
)

target_include_directories(ogg
    PRIVATE
        ${OGG_DIR}/include
)
