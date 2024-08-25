# CMakeList.txt: cmake 的 CMake 專案，在此包含來源及定義
# 專案專屬邏輯。
#
project(sdlpal_win32)

include_directories("../sdlpal/include")
include_directories("../external/SDL/include")
include_directories("../adplug")

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Debug)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Win32/Debug)

# 將來源新增到此專案的可執行檔。
add_executable (${PROJECT_NAME} "win32.cpp" "resource.h" "pal_config.h")
message("CMAKE_CURRENT_SOURCE_DIR:${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} 
  opengl32
  imm32
  winmm
  comctl32
  sdl2
  sdl2main
  native_midi
  libvorbis_static
  libogg
  timidity
  opus
  opusfile
  libmad
  libpal
  libbinio
  adplug
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

# TODO: 新增測試，並視需要安裝目標。
