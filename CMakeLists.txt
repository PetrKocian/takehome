cmake_minimum_required(VERSION 3.16)

project(network_config_demo LANGUAGES CXX)

add_library(network_config src/network_config.cpp)
target_include_directories(network_config PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(network_config PUBLIC cxx_std_20)
set_target_properties(network_config PROPERTIES CXX_EXTENSIONS OFF)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE network_config)
set_target_properties(demo PROPERTIES CXX_EXTENSIONS OFF)
