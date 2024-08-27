set (OPUSFILE_SOURCE_DIR external/opusfile/src)
set (OPUSFILE_INCLUDE_DIR external/opusfile/include)

add_library(opusfile
    ${OPUSFILE_SOURCE_DIR}/http.c
    ${OPUSFILE_SOURCE_DIR}/info.c
    ${OPUSFILE_SOURCE_DIR}/internal.c
    ${OPUSFILE_SOURCE_DIR}/opusfile.c
    ${OPUSFILE_SOURCE_DIR}/stream.c
    ${OPUSFILE_SOURCE_DIR}/wincerts.c
)

target_include_directories(opusfile
    PRIVATE
        ${OPUS_INCLUDE_DIR}
        ${OPUSFILE_INCLUDE_DIR}
        ${LIBOGG_INCLUDE_DIR}
)
