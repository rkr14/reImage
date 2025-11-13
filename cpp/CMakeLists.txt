cmake_minimum_required(VERSION 3.10)
project(GraphCutSegmenter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

add_executable(segment
    main.cpp
    Image.cpp
    SeedMask.cpp
    DataModel.cpp
    GraphBuilder.cpp
    Segmenter.cpp
    Dinic.cpp
    MinCut.h       # header-only helper
)

target_include_directories(segment PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
