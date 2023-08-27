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
# Compile
pushd $BUILD
    cmake .. -G Ninja
    ninja
    # ./CodeAnalysis ${JSON_PATH} ${SRC}/CodeAnalysis.cpp
    ./bin/CodeAnalysis /home/tz/test_kernel/kernel_source_code/linux-6.2.15/mm/zswap.c 2>&1 | tee ${LOG}/`date +%Y%m%d-%H%M%S`.log
popd