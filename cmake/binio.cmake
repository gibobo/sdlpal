add_library(binio
    STATIC
        ${BINIO_SOURCE_DIR}/binfile.cpp
        ${BINIO_SOURCE_DIR}/binio.cpp
        ${BINIO_SOURCE_DIR}/binstr.cpp
        ${BINIO_SOURCE_DIR}/binwrap.cpp
)

target_include_directories(binio
    PRIVATE
        ${BINIO_INCLUDE_DIR}
)
