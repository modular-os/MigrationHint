#!/bin/bash

# Basic Settings
BASE_PATH=$(pwd)
SRC=$BASE_PATH/src
BUILD=$BASE_PATH/build
LIB=$BASE_PATH/lib
INCLUDE=$BASE_PATH/include
LOG=$BASE_PATH/log
DATE=`date +%Y%m%d-%H%M%S`

TARGET_KERNEL=/home/tz/workspace/linux-kernel/linux-6.2.15
if [ ! -d $TARGET_KERNEL ]; then
    TARGET_KERNEL=/home/data/linux-6.5
fi

JSON_PATH=$TARGET_KERNEL/compile_commands.json 
JSON_PATH=$BASE_PATH/tmp/tmp.json

TARGET_SOURCE1=$TARGET_KERNEL/mm/zswap.c

TARGET_PROJ_PATH=/home/tz/workspace/linux-kernel/linux-6.2.15
TARGET_SOURCE1=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/zswap.c
# TARGET_SOURCE1=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/ksm.c
# TARGET_SOURCE=/home/tz/MigrationHint/test/simple_test_case1/test1.c
TARGET_SOURCE2=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/hugetlb.c
TARGET_SOURCE3=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/ksm.c
TARGET_SOURCE4=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/huge_memory.c
TARGET_SOURCE5=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/khugepaged.c
TARGET_SOURCE6=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/hugetlb_cgroup.c
TARGET_SOURCE7=/home/tz/workspace/linux-kernel/linux-6.2.15/mm/hugetlb_vmemmap.c
TARGET_SOURCE8=/home/tz/workspace/linux-kernel/linux-6.2.15/include/linux/ksm.h
TARGET_SOURCE_COMMANDS="-I./arch/x86/include -I./arch/x86/include/generated  -I./include -I./arch/x86/include/uapi -I./arch/x86/include/generated/uapi -I./include/uapi -I./include/generated/uapi"

######################### For Zeng

TARGET_FOLDER=/home/tz/workspace/glibcAnalysis/glibc-2.39
TARGET_SOURCE1=/home/tz/workspace/glibcAnalysis/glibc-2.39/sysdeps/hppa/dl-fptr.c

# Make a build dir if there isn't one
if [ ! -d $BUILD ]; then
    mkdir $BUILD
fi

# Not Matched commands
#  -include ./include/linux/compiler-version.h -include ./include/linux/kconfig.h -include ./include/linux/compiler_types.h

# Compile
pushd $BUILD
    cmake .. -G Ninja
    ninja
    set -x
    # --enable-function-analysis-by-headers \
    # --generate-report \
    # --enable-module-analysis \
    # --enable-migrate-code-gen \
    # --enable-function-analysis  \
    # --enable-struct-analysis \
    # ./bin/CodeAnalysis -s ${TARGET_SOURCE1} \
    #     --enable-json-gen \
    #     --enable-pp-analysis \
    #     -o ${LOG}/${DATE}.json \
    #     2>&1 | tee ${LOG}/${DATE}.log
    ./bin/CodeAnalysis -s ${TARGET_SOURCE1} -d ${TARGET_FOLDER} \
        --enable-support-yang \
        2>&1 | tee ${LOG}/${DATE}.log
    # ./bin/CAFlexParse --header \
    #     -i /home/tz/workspace/MigrationHint/log/20240522-030624.json \
    #     2>&1 | tee ${LOG}/front/${DATE}.c
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
# clang -cc1 -ast-dump -fsyntax-only \
#     ${TARGET_SOURCE_COMMANDS} \
#     ${TARGET_SOURCE} 2>&1 | tee log/zswap_ast.txt

# clang -cc1 -ast-dump -fsyntax-only \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/arch/x86/include \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/arch/x86/include/generated  \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/include \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/arch/x86/include/uapi \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/arch/x86/include/generated/uapi \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/include/uapi \
#     -I/home/tz/workspace/linux-kernel/linux-6.2.15/include/generated/uapi \
#     /home/tz/workspace/linux-kernel/linux-6.2.15/mm/zswap.c \
#     2>&1 | tee log/zswap_ast.txt

# View CFG
# clang -cc1 -analyze -analyzer-checker=debug.ViewCFG ${TARGET_SOURCE} 2>&1 | tee log/zswap_cfg.txt

# clang-query /home/tz/workspace/linux-kernel/linux-6.2.15/mm/zswap.c
# clang-query /home/tz/MigrationHint/test/simple_test_case1/test1.c

# Summarize the number of lines of code
# git ls-files | xargs wc -l