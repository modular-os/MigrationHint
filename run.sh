#!/bin/bash

# Basic Settings
BASE_PATH=$(pwd)
SRC=$BASE_PATH/src
BUILD=$BASE_PATH/build
LIB=$BASE_PATH/lib
INCLUDE=$BASE_PATH/include

JSON_PATH=/home/tz/test_kernel/kernel_source_code/linux-6.2.15/compile_commands.json 
JSON_PATH=/home/tz/MigrationHint/tmp/tmp.json
# Compile
pushd $BUILD
    cmake .. -G Ninja
    ninja
    ./CodeAnalysis ${JSON_PATH} ${SRC}/CodeAnalysis.cpp
popd