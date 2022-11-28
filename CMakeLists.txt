cmake_minimum_required(VERSION 3.0)
project(crdt VERSION 0.1.0 LANGUAGES CXX)

add_library(crdt INTERFACE)
target_include_directories(crdt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
