set (LIBMAD_DIR external/libmad)

add_library(libmad
  ${LIBMAD_DIR}/bit.c
  ${LIBMAD_DIR}/decoder.c
  ${LIBMAD_DIR}/fixed.c
  ${LIBMAD_DIR}/frame.c
  ${LIBMAD_DIR}/huffman.c
  ${LIBMAD_DIR}/layer12.c
  ${LIBMAD_DIR}/layer3.c
  ${LIBMAD_DIR}/stream.c
  ${LIBMAD_DIR}/synth.c
  ${LIBMAD_DIR}/timer.c
  )

target_include_directories(libmad
  PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${LIBMAD_DIR}
)

target_compile_definitions(libmad
    PRIVATE
        HAVE_CONFIG_H
        FPM_DEFAULT
)
