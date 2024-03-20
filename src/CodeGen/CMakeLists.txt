cmake_minimum_required(VERSION 3.16.3)
project(CodeGenProject)

set(LLVM_DIR /home/nakanomiku/CODES/modular_os/llvm-project-llvmorg-16.0.6/build/llvm16_install/lib/cmake/llvm)

# Error if commented out
SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")

find_package(LLVM REQUIRED CONFIG)

# Headers' path of LLVM
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Configure C++
set(CMAKE_CXX_COMPILER /home/nakanomiku/CODES/modular_os/llvm-project-llvmorg-16.0.6/build/llvm16_install/bin/clang++)

# Source File
add_executable(CodeGen src/cg_main.cpp)

# Link with LLVM libraries
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(CodeGen ${llvm_libs})

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
