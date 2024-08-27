add_library(libbinio
    STATIC
        ${LIBBINIO_SOURCE_DIR}/binfile.cpp
        ${LIBBINIO_SOURCE_DIR}/binio.cpp
        ${LIBBINIO_SOURCE_DIR}/binstr.cpp
        ${LIBBINIO_SOURCE_DIR}/binwrap.cpp
)

target_include_directories(libbinio
    PRIVATE
        ${LIBBINIO_INCLUDE_DIR}
)
