add_library(libogg
    ${LIBOGG_SOURCE_DIR}/bitwise.c
    ${LIBOGG_SOURCE_DIR}/framing.c
)

target_include_directories(libogg
    PRIVATE
        ${LIBOGG_INCLUDE_DIR}
)
