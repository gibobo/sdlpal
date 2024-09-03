file(GLOB ADPLUG_FILES
    ${ADPLUG_DIR}/*.cpp
    ${ADPLUG_DIR}/*/*.cpp
    ${NUKED_DIR}/*.c
    ${BINIO_SOURCE_DIR}/*.cpp
)

add_library(adplug STATIC ${ADPLUG_FILES})

target_include_directories(adplug 
    PRIVATE 
        ${ADPLUG_DIR}
        ${BINIO_SOURCE_DIR}
        ${BINIO_INCLUDE_DIR}
        ${NUKED_DIR}
    )