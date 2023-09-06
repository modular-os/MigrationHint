#!/bin/bash

# Basic Settings
BASE_PATH=$(pwd)
SRC=$BASE_PATH/src
BUILD=$BASE_PATH/build
LIB=$BASE_PATH/lib
INCLUDE=$BASE_PATH/include
LOG=$BASE_PATH/log

JSON_PATH=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/compile_commands.json 
JSON_PATH=/home/tz/MigrationHint/tmp/tmp.json

TARGET_PROJ_PATH=/home/tz/test_kernel/kernel_source_code/linux-6.2.15
TARGET_SOURCE=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/zswap.c
TARGET_SOURCE=/home/tz/MigrationHint/test/simple_test_case1/test1.c
TARGET_SOURCE_COMMANDS="-I./arch/x86/include -I./arch/x86/include/generated  -I./include -I./arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I./include/uapi -I./include/generated/uapi"

# Not Matched commands
#  -include ./include/linux/compiler-version.h -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h

# Compile
pushd $BUILD
    cmake .. -G Ninja
    ninja
    set -x
    ./bin/CodeAnalysis ${TARGET_SOURCE} 2>&1 | tee ${LOG}/`date +%Y%m%d-%H%M%S`.log
    set +x
popd

# Generate ast for target file
# C_INCLUDE_PATH=${TARGET_PROJ_PATH} \
# CPLUS_INCLUDE_PATH=${TARGET_PROJ_PATH} \
# clang -cc1 -ast-dump \
#     ${TARGET_SOURCE_COMMANDS} \
#     ${TARGET_SOURCE} 2>&1 | tee log/zswap_ast.txt

# View CFG
# clang -cc1 -analyze -analyzer-checker=debug.ViewCFG ${TARGET_SOURCE} 2>&1 | tee log/zswap_cfg.txt

# clang-query /home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/zswap.c
# clang-query /home/tz/MigrationHint/test/simple_test_case1/test1.c