set (ADPLUG_DIR adplug)
set (NUKED_DIR external/nuked)

add_library(adplug
  ${ADPLUG_DIR}/emuopls.cpp
  ${ADPLUG_DIR}/src/fprovide.cpp
  ${ADPLUG_DIR}/src/player.cpp
  ${ADPLUG_DIR}/src/rix.cpp
  ${ADPLUG_DIR}/src/surroundopl.cpp
  ${ADPLUG_DIR}/dosbox/dbopl.cpp
  ${ADPLUG_DIR}/dosbox/dosbox_opls.cpp
  ${ADPLUG_DIR}/dosbox/opl.cpp
  ${ADPLUG_DIR}/mame/fmopl.cpp
  ${ADPLUG_DIR}/mame/mame_opls.cpp
  ${ADPLUG_DIR}/mame/ymf262.cpp
  ${NUKED_DIR}/opl3.c
)

target_include_directories(adplug 
  PRIVATE 
    ${ADPLUG_DIR}
    ${SDL2_INCLUDE_DIR}    
    ${LIBBINIO_SOURCE_DIR}
    ${LIBBINIO_INCLUDE_DIR}
    ${NUKED_DIR}
)
