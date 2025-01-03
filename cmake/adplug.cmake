file(GLOB ADPLUG_FILES
    ${NUKED_DIR}/*.c
    ${BINIO_SOURCE_DIR}/*.cpp
    ${ADPLUG_DIR}/*.cpp
)

add_library(adplug STATIC ${ADPLUG_FILES})

target_include_directories(adplug 
    PRIVATE 
        ${BINIO_SOURCE_DIR}
        ${BINIO_INCLUDE_DIR}
        ${NUKED_DIR}
        ${ADPLUG_DIR}
    )