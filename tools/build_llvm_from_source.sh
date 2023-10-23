#!/bin/bash
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
LLVM_BUILD=${BASE_DIR}/llvm-project/build
LLVM_INSTALL=${BASE_DIR}/build/llvm16_install

# Remove duplicate build files
if [ -d ${LLVM_BUILD} ]; then
    rm -rf ${LLVM_BUILD}
fi
if [ -d ${LLVM_INSTALL} ]; then
    rm -rf ${LLVM_INSTALL}
fi

mkdir -p ${LLVM_BUILD}
mkdir -p ${LLVM_INSTALL}
pushd ${LLVM_BUILD}
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_TARGETS_TO_BUILD=host \
        -DLLVM_ENABLE_PROJECTS="clang;openmp;lldb;lld" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
        -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL}" \
        ../llvm/
    make -j20 && make install
popd
