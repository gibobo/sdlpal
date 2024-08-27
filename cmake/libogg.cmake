add_library(ogg
    ${LIBOGG_SOURCE_DIR}/bitwise.c
    ${LIBOGG_SOURCE_DIR}/framing.c
)

target_include_directories(ogg
    PRIVATE
        ${LIBOGG_INCLUDE_DIR}
)
