# MigrationHint

## 1 Introduction

MigrationHint is a tool based on Clang that offers suggestions during the process of migrating function modules from the Linux Kernel to other Unix-like operating systems.Key features:

- Out of tree - builds against a binary Clang installation (no need to build Clang from sources)
<!-- ### Table of Contents
* [2 Docs](#section1) -->
## 2 Docs

<!-- Reference to YUQUE Docs -->
You can refer to more develop details from [MigrationHint Docs](https://modular-os.yuque.com/org-wiki-modular-os-kfnn8q/eku5q1/saaga9w2ukbct33m).

## 3 Installation

### 3.1 Binaries
You can obtain the most up-to-date binaries by visiting the [Github Releases](https://github.com/modular-os/MigrationHint/releases) page. Please note that at present, only Linux x86_64 is supported.

Considering minimalistic runtime dependencies, the binary incorporates static linking with LLVM and Clang libraries. As a result, there is no requirement to separately install LLVM and Clang on your system while using binaries. However, it remains essential to install certain fundamental libraries. The following is a list of the runtime dependencies:
```bash
$ ldd build/bin/CodeAnalysis 
        linux-vdso.so.1 (0x0000155555518000)
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x0000155552652000)
        libz.so.1 => /lib/x86_64-linux-gnu/libz.so.1 (0x0000155552636000)
        libzstd.so.1 => /lib/x86_64-linux-gnu/libzstd.so.1 (0x0000155552567000)
        libtinfo.so.6 => /lib/x86_64-linux-gnu/libtinfo.so.6 (0x0000155552535000)
        libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (0x0000155552309000)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00001555522e7000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00001555520bf000)
        /lib64/ld-linux-x86-64.so.2 (0x000015555551a000)
```

### 3.2 Build from source

#### 3.2.1 Basic dependencies
To build CodeAnalysis and LLVM-Clang from source, it is necessary to install certain basic dependencies. Here are the requirements for Ubuntu and CentOS:

```bash
# Ubuntu
sudo apt update
sudo apt install build-essential cmake git
## Optional: Install the Clang static analyzer and clang-tidy
sudo apt install -y llvm-15 llvm-15-dev llvm-15-doc llvm-15-examples llvm-15-linker-tools llvm-15-tools llvm-dev libclang-15-dev

# CentOS
sudo yum install gcc gcc-c++ cmake git
sudo yum update
sudo yum groupinstall "Development Tools"
sudo yum install cmake git
## Optional: Install the Clang static analyzer and clang-tidy
sudo yum install llvm llvm-devel clang
```
Please note that these are basic requirements, and additional dependencies may be needed depending on your specific environment.

#### 3.2.2 LLVM and Clang
CodeAnalysis is based on LLVM LibTooling. To be more specific, it is based on the **LLVM-16**. 

Since our build system supports out-of-tree builds, it is not necessary to build LLVM from source. Instead, you can download the pre-built binaries from the [LLVM Download Page](https://releases.llvm.org/download.html). Please note that the LLVM version should be **LLVM-16**.

To ensure optimal compatibility with your system, it is recommended to build LLVM Clang from source. We have provided a helper script that facilitates the process. For more detailed instructions, you can refer to the following link: [Helper Script](tools/build_llvm_from_source.sh). This script will guide you through the steps required to build LLVM Clang from source and achieve a customized adaptation for your system.

#### 3.2.3 Build MigrationHint
The build system is based on CMake. You can build MigrationHint from source by following steps:

1. set `CA_Clang_INSTALL_DIR`: the path to the LLVM-16 installation directory.

2. cmake and make:

```bash
pushd $BUILD
    cmake .. -G Ninja
    ninja
popd
```

## 4 Usage
To utilize MigrationHint, it is essential to have the source code accompanied by a build system index in the form of `compile_commands.json`. For assistance in generating `compile_commands.json` while building the Linux kernel source code, you can refer to the provided [helper script](tools/build_linux_kernel_from_source.sh). This script will guide you through the process of building the Linux kernel source code and generating the necessary `compile_commands.json` file.


Please note that additional options and usage details can be found in the documentation or by running CodeAnalysis with the `--help` flag.
```bash
USAGE: CodeAnalysis [options]

OPTIONS:

Color Options:

  --color                               - Use colors in output (default=autodetect)

General options:

  --disable-i2p-p2i-opt                 - Disables inttoptr/ptrtoint roundtrip optimization
  --enable-function-analysis            - Enable external function analysis to source file
  --enable-function-analysis-by-headers - Enable external function analysis to source file, all the function declarations are grouped by header files.
  --enable-name-compression             - Enable name/filename string compression
  --enable-pp-analysis                  - Enable preprocess analysis to source file, show details of all the header files and macros.
  --enable-struct-analysis              - Enable external struct type analysis to source file
  --generate-merged-base-profiles       - When generating nested context-sensitive profiles, always generate extra base profile for function with all its context profiles merged into it.
  --opaque-pointers                     - Use opaque pointers
  -s <path-to-sourcefile>               - Path to the source file which is expected to be analyzed.

Generic Options:

  --help                                - Display available options (--help-hidden for more)
  --help-list                           - Display list of available options (--help-list-hidden for more)
  --version                             - Display the version of this program

Notice: 1. The compile_commands.json file should be in the same directory as the source file or in the parent directory of the source file.
        2. The Compilation Database should be named as compile_commands.json.
        3. You can input only one source file as you wish.

Developed by Zhe Tang<tangzh6101@gmail.com> for modular-OS project.
Version 1.0.0
```