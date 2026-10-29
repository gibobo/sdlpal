cmake_minimum_required(VERSION 3.16)
project(p4_landscape_tests LANGUAGES CXX)
add_executable(p4_landscape_tests tests.cpp
    ../../../driver/esp32/p4/landscape.cpp
    ../../../driver/esp32/p4/controls.cpp)
target_compile_features(p4_landscape_tests PRIVATE cxx_std_17)
enable_testing()
add_test(NAME p4_landscape COMMAND p4_landscape_tests)
