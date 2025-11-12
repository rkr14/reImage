cmake_minimum_required(VERSION 3.14)
project(reImage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(APPEND SRC_FILES
    "${CMAKE_SOURCE_DIR}/Segmentation/Segmentation/Dinic.cpp"
)

add_executable(segment ${SRC_FILES})

target_include_directories(segment PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Segmentation/Segmentation"
)


message(STATUS "Sources: ${SRC_FILES}")
