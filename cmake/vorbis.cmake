set (VORBIS_SOURCE_DIR external/vorbis/lib)
set (VORBIS_INCLUDE_DIR external/vorbis/include)

add_library(vorbis
    ${VORBIS_SOURCE_DIR}/analysis.c
    ${VORBIS_SOURCE_DIR}/bitrate.c
    ${VORBIS_SOURCE_DIR}/block.c
    ${VORBIS_SOURCE_DIR}/codebook.c
    ${VORBIS_SOURCE_DIR}/envelope.c
    ${VORBIS_SOURCE_DIR}/floor0.c
    ${VORBIS_SOURCE_DIR}/floor1.c
    ${VORBIS_SOURCE_DIR}/info.c
    ${VORBIS_SOURCE_DIR}/lookup.c
    ${VORBIS_SOURCE_DIR}/lpc.c
    ${VORBIS_SOURCE_DIR}/lsp.c
    ${VORBIS_SOURCE_DIR}/mapping0.c
    ${VORBIS_SOURCE_DIR}/mdct.c
    ${VORBIS_SOURCE_DIR}/psy.c
    ${VORBIS_SOURCE_DIR}/registry.c
    ${VORBIS_SOURCE_DIR}/res0.c
    ${VORBIS_SOURCE_DIR}/sharedbook.c
    ${VORBIS_SOURCE_DIR}/smallft.c
    ${VORBIS_SOURCE_DIR}/synthesis.c
    ${VORBIS_SOURCE_DIR}/vorbisenc.c
    ${VORBIS_SOURCE_DIR}/window.c
)

target_include_directories(vorbis
    PRIVATE
        ${VORBIS_INCLUDE_DIR}
        ${LIBOGG_INCLUDE_DIR}
)
