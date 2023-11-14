#!/bin/bash

# Basic Settings
BASE_PATH=$(pwd)
SRC=$BASE_PATH/src
BUILD=$BASE_PATH/build
LIB=$BASE_PATH/lib
INCLUDE=$BASE_PATH/include
LOG=$BASE_PATH/log

TARGET_KERNEL=/home/tz/test_kernel/kernel_source_code/linux-6.2.15
if [ ! -d $TARGET_KERNEL ]; then
    TARGET_KERNEL=/home/data/linux-6.5
fi

JSON_PATH=$TARGET_KERNEL/compile_commands.json 
JSON_PATH=$BASE_PATH/tmp/tmp.json

TARGET_SOURCE1=$TARGET_KERNEL/mm/zswap.c

TARGET_PROJ_PATH=/home/tz/test_kernel/kernel_source_code/linux-6.2.15
TARGET_SOURCE1=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/zswap.c
# TARGET_SOURCE1=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/ksm.c
# TARGET_SOURCE=/home/tz/MigrationHint/test/simple_test_case1/test1.c
TARGET_SOURCE2=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/hugetlb.c
TARGET_SOURCE3=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/ksm.c
TARGET_SOURCE4=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/huge_memory.c
TARGET_SOURCE5=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/khugepaged.c
TARGET_SOURCE_COMMANDS="-I./arch/x86/include -I./arch/x86/include/generated  -I./include -I./arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I./include/uapi -I./include/generated/uapi"

# Make a build dir if there isn't one
if [ ! -d $BUILD ]; then
    mkdir $BUILD
fi

# Not Matched commands
#  -include ./include/linux/compiler-version.h -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h

# Compile
pushd $BUILD
    cmake .. -G Ninja -D USE_CHINESE=ON
    ninja
    set -x
    # --enable-pp-analysis \
    # --enable-function-analysis-by-headers \
    ./bin/CodeAnalysis -s ${TARGET_SOURCE3} \
    --generate-report \
    --enable-struct-analysis \
    --enable-function-analysis  \
    2>&1 | tee ${LOG}/`date +%Y%m%d-%H%M%S`.log
    # ./bin/CodeAnalysis -s ${TARGET_SOURCE1},${TARGET_SOURCE2},${TARGET_SOURCE3},${TARGET_SOURCE4},${TARGET_SOURCE5} \
    # --enable-module-analysis \
    # 2>&1 | tee ${LOG}/`date +%Y%m%d-%H%M%S`.log
    # ./bin/CodeAnalysis -h
    # ./bin/CodeAnalysis ${TARGET_SOURCE1} ${TARGET_SOURCE2} 2>&1 | tee ${LOG}/`date +%Y%m%d-%H%M%S`.log
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

# Summarize the number of lines of code
# git ls-files | xargs wc -l