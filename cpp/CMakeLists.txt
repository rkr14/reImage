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

# Optimization flags for maximum performance with AVX2
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(segment PRIVATE
        -O3                    # Maximum optimization
        -march=native          # Use CPU-specific instructions
        -mavx2                 # Enable AVX2 instructions explicitly
        -mfma                  # Enable FMA (Fused Multiply-Add)
        -ffast-math           # Aggressive floating-point optimizations
        -funroll-loops        # Unroll loops where beneficial
        -finline-functions    # Aggressive inlining
        -ftree-vectorize      # Enable auto-vectorization
        -fopt-info-vec-optimized  # Report successful vectorizations
    )
    # Link-time optimization for GCC/Clang
    set_target_properties(segment PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
elseif(MSVC)
    target_compile_options(segment PRIVATE
        /O2                   # Maximize speed
        /Oi                   # Enable intrinsic functions
        /Ot                   # Favor fast code
        /GL                   # Whole program optimization
        /fp:fast              # Fast floating-point model
        /arch:AVX2            # Enable AVX2 instructions
    )
    set_target_properties(segment PROPERTIES
        LINK_FLAGS "/LTCG"    # Link-time code generation
    )
endif()
