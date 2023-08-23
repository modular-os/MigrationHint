cmake_minimum_required(VERSION 3.12)
project(CodeAnalysis)

set(CMAKE_CXX_STANDARD 14)

# 添加编译选项
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# 设置文件目录
set(SOURCE_DIR src)
set(INCLUDE_DIR include)
set(BINARY_DIR build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BINARY_DIR})

# 添加源文件
file(GLOB_RECURSE SOURCES ${SOURCE_DIR}/*.cpp)

# 添加头文件搜索路径
include_directories(${INCLUDE_DIR} )

# 添加执行文件
add_executable(CodeAnalysis ${SOURCES})

# 链接libclang库
find_package(Clang REQUIRED)
target_include_directories(CodeAnalysis PRIVATE ${CLANG_INCLUDE_DIRS})
target_link_libraries(CodeAnalysis PRIVATE ${CLANG_LIBRARIES})

# 链接llvm工具库
find_package(LLVM REQUIRED CONFIG)
## Add llvm components as needed, include libLLVMCore&libLLVMSupport。
llvm_map_components_to_libnames(LLVM_LIBS core support)
target_link_libraries(CodeAnalysis PRIVATE ${LLVM_LIBS})
target_include_directories(CodeAnalysis PRIVATE ${LLVM_INCLUDE_DIRS})

# 输出编译跳转命令compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)