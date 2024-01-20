#===============================================================================
# Description: CMakeLists.txt for the CodeAnalysis project.
#===============================================================================
# CMakeLists.txt for the CodeAnalysis project. 
# The CMakelists framework was originally inspired by the framework used in https://github.com/banach-space/clang-tutor
# Author: Zhe Tang
# Date:   2023-08-26
# Email:  tangzh6101[at]gmail.com

cmake_minimum_required(VERSION 3.16.3)
project(CodeAnalysis)

#===============================================================================
# 0. GET CLANG INSTALLATION DIR
#===============================================================================
# In CodeAnalysis, `CA_Clang_INSTALL_DIR` is the key CMake variable - it points
# to a Clang installation directory. CA means_ the special prefix for
# CodeAnalysis. Set this to a valid Clang installation directory. This is most 
# likely where LLVM is installed on your system.
set(CA_Clang_INSTALL_DIR /home/tz/applications/llvm/llvm16_install)
if(NOT EXISTS "${CA_Clang_INSTALL_DIR}")
  set(CA_Clang_INSTALL_DIR /usr/lib/llvm-16)
endif()

# Set the source, include and binary directories
set(CA_SOURCE_DIR src)
set(CA_INCLUDE_DIR include)
set(CA_BINARY_DIR build)
set(CA_EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR})

# Set the compiler
set(CMAKE_C_COMPILER ${CA_Clang_INSTALL_DIR}/bin/clang)
set(CMAKE_CXX_COMPILER ${CA_Clang_INSTALL_DIR}/bin/clang++)
message(STATUS "Using Clang++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Using Clang compiler: ${CMAKE_C_COMPILER}")

#===============================================================================
# 1. VERIFY CLANG INSTALLATION DIR
#===============================================================================
set(CA_LLVM_INCLUDE_DIR "${CA_Clang_INSTALL_DIR}/include/llvm")
if(NOT EXISTS "${CA_LLVM_INCLUDE_DIR}")
message(FATAL_ERROR
  " CA_Clang_INCLUDE_DIR (${CA_LLVM_INCLUDE_DIR}) is invalid.")
endif()

set(CA_LLVM_CMAKE_FILE
  "${CA_Clang_INSTALL_DIR}/lib/cmake/clang/ClangConfig.cmake")
if(NOT EXISTS "${CA_LLVM_CMAKE_FILE}")
message(FATAL_ERROR
  " CA_LLVM_CMAKE_FILE (${CA_LLVM_CMAKE_FILE}) is invalid.")
endif()

#===============================================================================
# 2. LOAD CLANG CONFIGURATION
#    Extracted from:
#    http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
#===============================================================================
list(APPEND CMAKE_PREFIX_PATH "${CA_Clang_INSTALL_DIR}/lib/cmake/clang/")

find_package(Clang REQUIRED CONFIG)

# Sanity check. As Clang does not expose e.g. `CLANG_VERSION_MAJOR` through
# AddClang.cmake, we have to use LLVM_VERSION_MAJOR instead.
# TODO: Revisit when next version is released.
if(NOT "16" VERSION_EQUAL "${LLVM_VERSION_MAJOR}")
  message(FATAL_ERROR "Found LLVM ${LLVM_VERSION_MAJOR}, but need LLVM 16")
endif()

message(STATUS "Found Clang ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${CA_Clang_INSTALL_DIR}")

message("CLANG STATUS:
  Includes (clang)    ${CLANG_INCLUDE_DIRS}
  Includes (llvm)     ${LLVM_INCLUDE_DIRS}"
)

# Set the LLVM and Clang header and library paths
include_directories(SYSTEM "${LLVM_INCLUDE_DIRS};${CLANG_INCLUDE_DIRS}")

#===============================================================================
# 3. CodeAnalysis BUILD CONFIGURATION
#===============================================================================
# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE
      STRING "Build type (default Debug):" FORCE)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g\
    -fdiagnostics-color=always")

# Settings for using Chinese output
option(USE_CHINESE "Use Chinese output" OFF)
if(USE_CHINESE)
    message(STATUS "Using Chinese output")
    add_definitions(-DCHN)
endif()

# LLVM/Clang is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Set the build directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# Generate compile_commands.json for use with clang_complete etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#===============================================================================
# 4. ADD SUB-TARGETS
# Doing this at the end so that all definitions and link/include paths are
# available for the sub-projects.
#===============================================================================
add_subdirectory(src)