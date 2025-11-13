cmake_minimum_required(VERSION 3.20)

project(reImage LANGUAGES CXX)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(segment
  src/main.cpp
  src/Image.cpp
  src/SeedMask.cpp
  src/DataModel.cpp
  src/GraphBuilder.cpp
)

target_link_libraries(segment PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(segment PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Build instructions:
#   conan install . --output-folder=build/conan --build=missing
#   cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
#   cmake --build build -j

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(segment PRIVATE
        -O3       
        -march=native          
        -flto                  
        -ffast-math           
        -funroll-loops        
    )
elseif(MSVC)
    target_compile_options(segment PRIVATE
        /O2 /Oi /Ot /GL  
        /arch:AVX2  
    )
    set_target_properties(segment PROPERTIES LINK_FLAGS "/LTCG")
endif()
